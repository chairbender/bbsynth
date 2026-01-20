#include "OTAFilterDelayedFeedback.h"

#include <juce_dsp/juce_dsp.h>

#include <ranges>

#include "../Constants.h"
#include "../Utils.h"

namespace audio_plugin {
OTAFilterDelayedFeedback::OTAFilterDelayedFeedback(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::AudioBuffer<float>& env_buffer,
    const juce::AudioBuffer<float>& lfo_buffer)
    : ParameterListenerManager{apvts},
      cutoff_freq_{0.f},
      resonance_{0.f},
      drive_{0.f},
      env_mod_{0.f},
      lfo_mod_{0.f},
      num_stages_{4},
      env_buffer_{&env_buffer},
      lfo_buffer_{lfo_buffer},
      sample_rate_{0},
      s1_{0},
      s2_{0},
      s3_{0},
      s4_{0},
      dc_blocker_{0.99f} {
  AddParameterListener("filterCutoffFreq", FilterParamId::kCutoffFreq,
                       [this](const float value) {
                         cutoff_freq_ = value;
                       });
  AddParameterListener("filterResonance", FilterParamId::kResonance,
                       [this](const float value) {
                         resonance_ = value;
                       });
  AddParameterListener("filterDrive", FilterParamId::kDrive,
                       [this](const float value) {
                         drive_ = value;
                       });
  AddParameterListener("filterEnvMod", FilterParamId::kEnvMod,
                       [this](const float value) {
                         env_mod_ = value;
                       });
  AddParameterListener("filterLfoMod", FilterParamId::kLfoMod,
                       [this](const float value) {
                         lfo_mod_ = value;
                       });

  for (const auto [i, input_drive_param] :
       std::views::enumerate(kInputDriveScaleParams)) {
    AddParameterListener(input_drive_param, static_cast<FilterParamId>(static_cast<int>(FilterParamId::kInputDriveScale1) + i),
                         [this, i](const float value) {
                           input_drive_scales_[i] = value;
                         });
  }
  for (const auto [i, state_drive_param] :
       std::views::enumerate(kStateDriveScaleParams)) {
    AddParameterListener(state_drive_param, static_cast<FilterParamId>(static_cast<int>(FilterParamId::kStateDriveScale1) + i),
                         [this, i](const float value) {
                           state_drive_scales_[i] = value;
                         });
  }

  AddParameterListener("filterSlope", FilterParamId::kFilterSlope,
                       [this](const float value) {
                         switch (static_cast<int>(value)) {
                           case 0:
                             num_stages_ = 4;
                             break;
                           case 1:
                             num_stages_ = 3;
                             break;
                           case 2:
                             num_stages_ = 2;
                             break;
                           default:
                             num_stages_ = 4;
                             break;
                         }
                       });
}

inline void OTAFilterDelayedFeedback::FilterStage(const float in, float& out,
                                                  TanhADAA& tanh_in,
                                                  TanhADAA& tanh_state,
                                                  const float g,
                                                  const float scale) const {
  constexpr auto kLeak = 0.99995f;
  const auto stage_index = &tanh_in - &tanh_in_[0];
  const auto state_scale =
      1.f / (drive_ * state_drive_scales_[static_cast<size_t>(stage_index)]);
  const auto tanh_in_val = tanh_in.process(in * scale);
  const auto tanh_state_val = tanh_state.process(out * state_scale);
  const float v = tanh_in_val * (1.f / scale);
  out = Sanitize(kLeak * out + g * (v - tanh_state_val * (1.f / state_scale)));
}

void OTAFilterDelayedFeedback::Process(juce::AudioBuffer<float>& buffers,
                                       const int start_sample,
                                       const int numSamples) {
  ProcessDirtyParameters();
  jassert(sample_rate_ > 0);

  // todo vectorize
  const auto samples = std::span(buffers.getWritePointer(0) + start_sample,
                                 static_cast<size_t>(numSamples));
  const auto sample_chunks = samples | std::ranges::views::chunk(kOversample);
  const auto env_data =
      std::span(env_buffer_->getReadPointer(0) + start_sample / kOversample,
                static_cast<size_t>(numSamples / kOversample));
  const auto lfo_data =
      std::span(lfo_buffer_.getReadPointer(0) + start_sample / kOversample,
                static_cast<size_t>(numSamples / kOversample));

  for (const auto [sample_chunk, env_sample, lfo_sample] :
       std::views::zip(sample_chunks, env_data, lfo_data)) {
    for (auto& sample : sample_chunk) {
      // modulation - envelope and LFO affects cutoff frequency
      const float modulated_cutoff =
          juce::jlimit(kMinCutoff, kMaxCutoff,
                       cutoff_freq_ + env_mod_ * env_sample * kMaxCutoff +
                           lfo_mod_ * lfo_sample * kMaxCutoff);

      // this was my original "naive" approach which can exceed 1 in some cases
      // and blow the filter up. It seems to work fine now that I've addressed
      // other issues with the filter.
      const auto g = tanf(juce::MathConstants<float>::pi * modulated_cutoff /
                          static_cast<float>(sample_rate_));
      // this TPT method of calculating g ensures the value won't exceed 1.
      // const auto g = tanf(juce::MathConstants<float>::pi *
      // modulated_cutoff/static_cast<float>(sample_rate_)) /
      //  (1 + tanf(juce::MathConstants<float>::pi *
      //  modulated_cutoff/static_cast<float>(sample_rate_)));
      // this approach simply clamps g to ensure it doesn't exceed 1
      // const auto g = std::min(.9f, std::tanf(juce::MathConstants<float>::pi *
      // modulated_cutoff/static_cast<float>(sample_rate_)));

      // resonance feedback from output
      float last_stage_output = 0;
      switch (num_stages_) {
        case 1:
          last_stage_output = s1_;
          break;
        case 2:
          last_stage_output = s2_;
          break;
        case 3:
          last_stage_output = s3_;
          break;
        case 4:
          last_stage_output = s4_;
          break;
        default:
          last_stage_output = s4_;
          break;
      }

      // feedback with compensation
      // const auto feedback = resonance_ * last_stage_output / (1 + resonance_
      // * (1.f / static_cast<float>(num_stages_))); feedback without
      // compensation
      const auto feedback = resonance_ * last_stage_output;

      // input with soft clipping
      // try 0.8 to 1.5 range
      // todo: we could even expose this as yet another param
      constexpr auto kFeedbackDrive = 1.f;
      constexpr auto kFeedbackScale = 1.f / kFeedbackDrive;
      const auto u =
          sample -
          tanh_feedback_.process(feedback * kFeedbackScale) * kFeedbackDrive;

      // todo: different scale / drive amount for each stage as opposed to the
      // same for each.

      FilterStage(u, s1_, tanh_in_[0], tanh_state_[0], g,
                  1.f / (drive_ * input_drive_scales_[0]));
      if (num_stages_ >= 2)
        FilterStage(s1_, s2_, tanh_in_[1], tanh_state_[1], g,
                    1.f / (drive_ * input_drive_scales_[1]));
      if (num_stages_ >= 3)
        FilterStage(s2_, s3_, tanh_in_[2], tanh_state_[2], g,
                    1.f / (drive_ * input_drive_scales_[2]));
      if (num_stages_ >= 4)
        FilterStage(s3_, s4_, tanh_in_[3], tanh_state_[3], g,
                    1.f / (drive_ * input_drive_scales_[3]));

      // DC block and soft clip the output
      // try 2.0 - 4.0 range
      // todo: we could even expose this as yet another param
      constexpr auto kOutputDrive = 2.f;
      constexpr auto kOutputScale = 1.f / kOutputDrive;
      // prevents the clipping inherent in the TanhADAA calculation
      // (happens at extreme g, res, drive values)
      const auto tanh_final_out_val =
          tanh_final_out_.process((last_stage_output) * kOutputScale);
      // it can very slightly clip, but that's within tolerable levels, so
      // we don't clamp here.
      sample = dc_blocker_.Process(tanh_final_out_val);
    }
  }
}

void OTAFilterDelayedFeedback::Reset() {
  s1_ = s2_ = s3_ = s4_ = 0;
  dc_blocker_.Reset();
  tanh_final_out_.reset();
  tanh_feedback_.reset();
  for (auto& tanh : tanh_state_) tanh.reset();
  for (auto& tanh : tanh_in_) tanh.reset();
}

void OTAFilterDelayedFeedback::set_sample_rate(const double rate) {
  sample_rate_ = static_cast<float>(rate);
}
}  // namespace audio_plugin