#include "BBSynth/OTAFilter.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>

#include "BBSynth/Constants.h"

namespace audio_plugin {
OTAFilter::OTAFilter() : cutoff_freq_{0.f}, resonance_{0.f},
                         drive_{0.f}, env_mod_{0.f}, lfo_mod_{0.f}, num_stages_{4}, bypass_{false}, sample_rate_{0}, s1_{0},
                         s2_{0}, s3_{0},
                         s4_{0},
                         dc_out_x1_{0},
                         dc_out_y1_{0} {
}

inline void OTAFilter::FilterStage(const float in, float& out,
                                   TanhADAA& tanh_in, TanhADAA& tanh_state,
                                   const float g, const float scale) const {
  constexpr auto kLeak = 0.99995f;
  const auto tanh_in_val = tanh_in.process(in * scale);
  const auto tanh_state_val = tanh_state.process(out * scale);
  const float v = tanh_in_val * drive_;
  out = kLeak * out + g * (v - tanh_state_val * drive_);
}

void OTAFilter::Process(juce::AudioBuffer<float>& buffers,
                        const juce::AudioBuffer<float>& env_buffer,
                        const juce::AudioBuffer<float>& lfo_buffer,
                        const int start_sample,
                        const int numSamples) {
  if (bypass_) {
    return;
  }
  jassert(sample_rate_ > 0);

  // todo vectorize
  const auto scale = 1.f / drive_;
  const auto buf = buffers.getWritePointer(0);
  const auto env_data = env_buffer.getReadPointer(0);
  const auto lfo_data = lfo_buffer.getReadPointer(0);

  for (auto i = start_sample; i < numSamples; ++i) {
    const auto sample = buf[i];
    // modulation - envelope and LFO affects cutoff frequency
    // todo: decide on a reasonable modulation range/curve
    // for now, let's say env_mod is -1 to 1, and it can shift cutoff by some amount
    // cutoff_freq_ is 20 to 8000
    // let's try adding env_mod_ * env_data[i] * range + lfo_mod_ * lfo_data[i] * range
    // prevent exceeding the nyquist
    const float modulated_cutoff = juce::jlimit(kMinCutoff, static_cast<float>(sample_rate_) * 0.49f, cutoff_freq_ + env_mod_ * env_data[i / kOversample] * 4000.f + lfo_mod_ * lfo_data[i / kOversample] * 4000.f);

    // this was my original "naive" approach which can exceed 1 in some cases and blow the filter up.
    // It seems to work fine now that I've addressed other issues with the filter.
    const auto g = tanf(juce::MathConstants<float>::pi * modulated_cutoff / static_cast<float>(sample_rate_));
    // this TPT method of calculating g ensures the value won't exceed 1.
    //const auto g = tanf(juce::MathConstants<float>::pi * modulated_cutoff/static_cast<float>(sample_rate_)) /
    //  (1 + tanf(juce::MathConstants<float>::pi * modulated_cutoff/static_cast<float>(sample_rate_)));
    // this approach simply clamps g to ensure it doesn't exceed 1
    //const auto g = std::min(.9f, std::tanf(juce::MathConstants<float>::pi * modulated_cutoff/static_cast<float>(sample_rate_)));

    if (g >= 1) {
      DBG("g exceeded 1 " + juce::String(g));
    }

    // resonance feedback from output
    float last_stage_output = 0;
    switch (num_stages_) {
      case 1: last_stage_output = s1_; break;
      case 2: last_stage_output = s2_; break;
      case 3: last_stage_output = s3_; break;
      case 4: last_stage_output = s4_; break;
      default: last_stage_output = s4_; break;
    }

    // feedback with compensation
    //const auto feedback = resonance_ * last_stage_output / (1 + resonance_ * (1.f / static_cast<float>(num_stages_)));
    // feedback without compensation
    const auto feedback = resonance_ * last_stage_output;

    // input with soft clipping
    // try 0.8 to 1.5 range
    // todo: we could even expose this as yet another param
    const auto kFeedbackDrive = 1.f;
    const auto u = sample - tanh_feedback_.process(feedback / kFeedbackDrive) * kFeedbackDrive;

    // todo: different scale for each stage

    FilterStage(u, s1_, tanh_in_[0], tanh_state_[0], g, scale);
    if (num_stages_ >= 2) FilterStage(s1_, s2_, tanh_in_[1], tanh_state_[1], g, scale);
    if (num_stages_ >= 3) FilterStage(s2_, s3_, tanh_in_[2], tanh_state_[2], g, scale);
    if (num_stages_ >= 4) FilterStage(s3_, s4_, tanh_in_[3], tanh_state_[3], g, scale);

    // DC block and soft clip the output
    // try 2.0 - 4.0 range
    // todo: we could even expose this as yet another param
    constexpr auto kOutputDrive = 2.f;
    buf[i] = tanh_final_out_.process((last_stage_output - dc_out_x1_ + 0.99f * dc_out_y1_) / kOutputDrive) * kOutputDrive;
    dc_out_x1_ = last_stage_output;
    dc_out_y1_ = buf[i];
  }
}

void OTAFilter::Configure(const juce::AudioProcessorValueTreeState& state) {
  cutoff_freq_ = state.getRawParameterValue("filterCutoffFreq")->load();
  resonance_ = state.getRawParameterValue("filterResonance")->load();
  drive_ = state.getRawParameterValue("filterDrive")->load();
  env_mod_ = state.getRawParameterValue("filterEnvMod")->load();
  lfo_mod_ = state.getRawParameterValue("filterLfoMod")->load();
  bypass_ = state.getRawParameterValue("vcfBypass")->load() > 0.5f;
  switch (static_cast<int>(
              state.getRawParameterValue("filterSlope")->load())) {
    case 0: num_stages_ = 4; break;
    case 1: num_stages_ = 3; break;
    case 2: num_stages_ = 2; break;
    default: num_stages_ = 4; break;
  }
}

void OTAFilter::Reset() {
  s1_ = s2_ = s3_ = s4_ = 0;
  dc_out_x1_ = dc_out_y1_ = 0;
  tanh_final_out_.reset();
  tanh_feedback_.reset();
  for (auto& tanh : tanh_state_) tanh.reset();
  for (auto& tanh : tanh_in_) tanh.reset();
}

void OTAFilter::set_sample_rate(const double rate) {
  sample_rate_ = static_cast<float>(rate);
}
}