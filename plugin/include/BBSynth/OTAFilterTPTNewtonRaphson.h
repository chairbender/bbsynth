#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <vector>

#include "TanhADAA.h"

namespace audio_plugin {

/**
 * 4 pole, mono, OTA filter emulation with adjustable drive.
 * Unlike the OTAFilterDelayedFeedback, this uses a TPT approach with
 * Newton-Raphson iteration to iteratively solve the implicit ODE for each
 * sample, which is more accurate, avoids introducing delay in the feedback, but
 * also is way more computationally expensive.
 */
class OTAFilterTPTNewtonRaphson {
 public:
  OTAFilterTPTNewtonRaphson(const juce::AudioBuffer<float>& env_buffer,
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
  // TODO: not used yet
  int num_stages_;

 private:
  float ProcessSample(float in, float env_val, float lfo_val);

  //Saturation function
  float Saturate(float x) const;
  // Derivative of saturation function for newton raphson
  float SaturateDerivative(float x) const;
  // Evaluate filter for a given output guess.
  // Returns what the output would be if the actual output were 'out_guess'
  float EvaluateFilter(float in, float out_guess, float G, float k,
    float& v1_out, float& v2_out, float& v3_out, float& v4_out) const;
  // Compute derivative for newton raphson
  // d(output)/d(out_guess) = how much does changing our guess change the predicted output?
  float ComputeJacobian(float in, float out_guess, float G, float k) const;

  const juce::AudioBuffer<float>* env_buffer_;
  const juce::AudioBuffer<float>& lfo_buffer_;
  float sample_rate_;
  // state vars for each stage
  float s1_, s2_, s3_, s4_;
  // dc blocker
  // todo: convert all my DC blockers to juse use juce builtin filters
  //todo float dc_out_x1_, dc_out_y1_;
};

}  // namespace audio_plugin