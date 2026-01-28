#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>

#include "../ParameterListenerManager.h"
#include "../dsp/DCBlocker.h"
#include "../dsp/TanhADAA.h"
#include "OTAFilterTPTNewtonRaphson.h"

namespace audio_plugin {

/**
 * 4 pole, mono, OTA filter emulation with adjustable drive.
 * This has a one-sample delay for the feedback as it does not use a TPT
 * approach, thus it is not quite as analog accurate.
 */
class OTAFilterDelayedFeedback
    : public ParameterListenerManager<OTAFilterDelayedFeedback, FilterParamId> {
 public:
  OTAFilterDelayedFeedback(juce::AudioProcessorValueTreeState& apvts,
                           const juce::AudioBuffer<float>& env1_buffer,
                           const juce::AudioBuffer<float>& env2_buffer,
                           const juce::AudioBuffer<float>& lfo_buffer);
  /**
   * Perform in place filtering on the left channel only,
   * for numSamples samples.
   */
  void Process(const juce::dsp::AudioBlock<float>& buffers, int start_sample,
               int numSamples);

  void Reset();
  void set_sample_rate(double rate);
  void set_use_env1(bool env1);

  float cutoff_freq_;
  float resonance_;
  // value of zero disables the distortion
  float drive_;
  float env_mod_;
  float lfo_mod_;
  int num_stages_;
  std::array<float, 4> input_drive_scales_;
  std::array<float, 4> state_drive_scales_;

 private:
  void FilterStage(float in, float& out, TanhADAA& tanh_in,
                   TanhADAA& tanh_state, float g, float scale) const;

  const juce::AudioBuffer<float>& env1_buffer_;
  const juce::AudioBuffer<float>& env2_buffer_;
  bool use_env1_ = true;
  const juce::AudioBuffer<float>& lfo_buffer_;
  float sample_rate_;
  // integrator states
  float s1_, s2_, s3_, s4_;
  // dc blocker
  DCBlocker dc_blocker_;
  // ADAA tanh
  std::array<TanhADAA, 4> tanh_in_;
  std::array<TanhADAA, 4> tanh_state_;
  TanhADAA tanh_final_out_;
  TanhADAA tanh_feedback_;
};

}  // namespace audio_plugin