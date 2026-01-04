#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <vector>

#include "TanhADAA.h"

namespace audio_plugin {

/**
 * 4 pole, mono, OTA filter emulation with adjustable drive.
 * This has a one-sample delay for the feedback as it does not use a TPT
 * approach, thus it is not quite as analog accurate.
 */
class OTAFilterDelayedFeedback {
 public:
  OTAFilterDelayedFeedback(const juce::AudioBuffer<float>& env_buffer,
                           const juce::AudioBuffer<float>& lfo_buffer);
  /**
   * Perform in place filtering on the left channel only,
   * for numSamples samples.
   */
  void Process(juce::AudioBuffer<float>& buffers, int start_sample,
               int numSamples);

  void set_env_buffer(const juce::AudioBuffer<float>& env_buffer) {
    env_buffer_ = &env_buffer;
  }

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
  // value of zero disables the distortion
  float drive_;
  float env_mod_;
  float lfo_mod_;
  int num_stages_;

 private:
  void FilterStage(float in, float& out, TanhADAA& tanh_in,
                   TanhADAA& tanh_state, float g, float scale) const;

  const juce::AudioBuffer<float>* env_buffer_;
  const juce::AudioBuffer<float>& lfo_buffer_;
  float sample_rate_;
  // integrator states
  float s1_, s2_, s3_, s4_;
  // dc blocker
  float dc_out_x1_, dc_out_y1_;
  // ADAA tanh
  std::array<TanhADAA, 4> tanh_in_;
  std::array<TanhADAA, 4> tanh_state_;
  TanhADAA tanh_final_out_;
  TanhADAA tanh_feedback_;
};

}  // namespace audio_plugin