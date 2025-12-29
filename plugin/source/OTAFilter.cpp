#include "BBSynth/OTAFilter.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>

#include "BBSynth/Constants.h"

namespace audio_plugin {
OTAFilter::OTAFilter() : cutoff_freq_{0.f}, resonance_{0.f},
                         drive_{0.f}, env_mod_{0.f}, lfo_mod_{0.f}, num_stages_{4}, sample_rate_{0}, s1_{0},
                         s2_{0}, s3_{0},
                         s4_{0},
                         dc_out_x1_{0},
                         dc_out_y1_{0} {
}

inline void OTAFilter::FilterStage(const float in, float& out,
                                   TanhADAA& tanh_in, TanhADAA& tanh_state,
                                   const float g, const float scale) const {
  constexpr auto kLeak = 0.99995f;
  const float v = drive_ > 0.f ? ( tanh_in.process(in * scale) * drive_) : in;
  out = kLeak * out + g * (v - (drive_ > 0.f ? (tanh_state.process(
                               out * scale) * drive_) : out));
}

void OTAFilter::Process(juce::AudioBuffer<float>& buffers,
                        const juce::AudioBuffer<float>& env_buffer,
                        const juce::AudioBuffer<float>& lfo_buffer,
                        const int numSamples) {
  jassert(sample_rate_ > 0);

  // todo vectorize
  const auto scale = drive_ > 0.f ? 1.f / drive_ : 1.f;
  // leaky integrator for numerical stability
  const auto buf = buffers.getWritePointer(0);
  const auto env_data = env_buffer.getReadPointer(0);
  const auto lfo_data = lfo_buffer.getReadPointer(0);

  for (auto i = 0; i < numSamples; ++i) {
    const auto sample = buf[i];
    // modulation - envelope and LFO affects cutoff frequency
    // todo: decide on a reasonable modulation range/curve
    // for now, let's say env_mod is -1 to 1, and it can shift cutoff by some amount
    // cutoff_freq_ is 20 to 8000
    // let's try adding env_mod_ * env_data[i] * range + lfo_mod_ * lfo_data[i] * range
    const float modulated_cutoff = juce::jlimit(20.f, 20000.f, cutoff_freq_ + env_mod_ * env_data[i / kOversample] * 4000.f + lfo_mod_ * lfo_data[i / kOversample] * 4000.f);
    const auto g = tanf(juce::MathConstants<float>::pi * modulated_cutoff / static_cast<float>(sample_rate_));

    // resonance feedback from output
    float last_stage_output = 0;
    switch (num_stages_) {
      case 1: last_stage_output = s1_; break;
      case 2: last_stage_output = s2_; break;
      case 3: last_stage_output = s3_; break;
      case 4: last_stage_output = s4_; break;
      default: last_stage_output = s4_; break;
    }
    // TODO: weird behavior when res set to 0...
    const auto feedback = resonance_ * last_stage_output;

    // input with feedback compensation
    const auto u = sample - feedback;

    // todo: different scale for each stage

    FilterStage(u, s1_, tanh_in_[0], tanh_state_[0], g, scale);
    if (num_stages_ >= 2) FilterStage(s1_, s2_, tanh_in_[1], tanh_state_[1], g, scale);
    if (num_stages_ >= 3) FilterStage(s2_, s3_, tanh_in_[2], tanh_state_[2], g, scale);
    if (num_stages_ >= 4) FilterStage(s3_, s4_, tanh_in_[3], tanh_state_[3], g, scale);

    // DC block the output
    buf[i] = last_stage_output - dc_out_x1_ + 0.99f * dc_out_y1_;
    dc_out_x1_ = last_stage_output;
    dc_out_y1_ = buf[i];
  }
}

void OTAFilter::Configure(const juce::AudioProcessorValueTreeState& state) {
  cutoff_freq_ = state.getRawParameterValue("filterCutoffFreq")->load();
  // todo: "0" resonance should actually be somewhere between .5 and 1 - such as .707.
  resonance_ = state.getRawParameterValue("filterResonance")->load();
  drive_ = state.getRawParameterValue("filterDrive")->load();
  env_mod_ = state.getRawParameterValue("filterEnvMod")->load();
  lfo_mod_ = state.getRawParameterValue("filterLfoMod")->load();
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
  for (auto& tanh : tanh_state_) tanh.reset();
  for (auto& tanh : tanh_in_) tanh.reset();
}

void OTAFilter::set_sample_rate(const double rate) {
  sample_rate_ = static_cast<float>(rate);
}
}