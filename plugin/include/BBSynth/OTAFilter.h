#pragma once

#include "TanhADAA.h"
#include <array>

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace audio_plugin {

/**
 * 4 pole, mono, OTA filter emulation
 */
class OTAFilter {
public:
  OTAFilter();
  /**
   * Perform in place filtering on the left channel only,
   * for numSamples samples.
   */
  void Process(juce::AudioBuffer<float>& buffers, int numSamples);

  /**
   * Update params based on current state
   */
  void Configure(const juce::AudioProcessorValueTreeState& state);

  /**
   * Reset for next note
   */
  void Reset();
  void set_sample_rate(double rate);

  float cutoff_freq_;
  float resonance_;
  float drive_;

private:
  void FilterStage(float in, float& out, TanhADAA& tanh_in, TanhADAA& tanh_state, float g, float scale) const;

  float sample_rate_;
  // integrator states
  float s1_, s2_, s3_, s4_;
  // dc blocker
  float dc_out_x1_, dc_out_y1_;
  // ADAA tanh
  std::array<TanhADAA, 4> tanh_in_;
  std::array<TanhADAA, 4> tanh_state_;
};

}