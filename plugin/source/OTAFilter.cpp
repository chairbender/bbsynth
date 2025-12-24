#include "BBSynth/OTAFilter.h"
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {
OTAFilter::OTAFilter() : cutoff_freq_{0.f}, resonance_{0.f},
                         drive_{0.f}, sample_rate_{0}, s1_{0},
                         s2_{0}, s3_{0},
                         s4_{0},
                         dc_out_x1_{0},
                         dc_out_y1_{0} {
}

inline void OTAFilter::FilterStage(const float in, float& out,
                                   TanhADAA& tanh_in, TanhADAA& tanh_state,
                                   const float g, const float scale) const {
  constexpr auto kLeak = 0.99995f;
  const auto v = tanh_in.process(in * scale) * drive_;
  out = kLeak * out + g * (v - tanh_state.process(
                               out * scale) * drive_);
}

void OTAFilter::Process(juce::AudioBuffer<float>& buffers,
                        const int numSamples) {
  jassert(sample_rate_ > 0);

  // todo vectorize
  const auto g = tanf(
      juce::MathConstants<float>::pi * cutoff_freq_ / static_cast<float>(
        sample_rate_));
  const auto scale = 1.f / drive_;
  // leaky integrator for numerical stability
  const auto buf = buffers.getWritePointer(0);

  for (auto i = 0; i < numSamples; ++i) {
    const auto sample = buf[i];

    // resonance feedback from output
    const auto feedback = resonance_ * s4_;

    // input with feedback compensation
    const auto u = sample - feedback;

    // todo: different scale for each stage

    FilterStage(u, s1_, tanh_in_[0], tanh_state_[0], g, scale);
    FilterStage(s1_, s2_, tanh_in_[1], tanh_state_[1], g, scale);
    FilterStage(s2_, s3_, tanh_in_[2], tanh_state_[2], g, scale);
    FilterStage(s3_, s4_, tanh_in_[3], tanh_state_[3], g, scale);

    // DC block the output
    buf[i] = s4_ - dc_out_x1_ + 0.99f * dc_out_y1_;
    dc_out_x1_ = s4_;
    dc_out_y1_ = buf[i];
  }
}

void OTAFilter::Configure(const juce::AudioProcessorValueTreeState& state) {
  cutoff_freq_ = state.getRawParameterValue("filterCutoffFreq")->load();
  resonance_ = state.getRawParameterValue("filterResonance")->load();
  drive_ = state.getRawParameterValue("filterDrive")->load();
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