#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <ranges>

#include "../ParameterListenerManager.h"
#include "../dsp/TanhADAA.h"

namespace audio_plugin {

enum class FilterParamId {
  kCutoffFreq,
  kResonance,
  kDrive,
  kEnvMod,
  kLfoMod,
  kInputDriveScale1,
  kInputDriveScale2,
  kInputDriveScale3,
  kInputDriveScale4,
  kStateDriveScale1,
  kStateDriveScale2,
  kStateDriveScale3,
  kStateDriveScale4,
  kFilterSlope,
  kCount
};

/**
 * 4 pole, mono, OTA filter emulation with adjustable drive.
 * Unlike the OTAFilterDelayedFeedback, this uses a TPT approach with
 * Newton-Raphson iteration to iteratively solve the implicit ODE for each
 * sample, which is more accurate, avoids introducing delay in the feedback, but
 * also is way more computationally expensive.
 */
// TODO: lot of stuff to dedupe between the 2 filter types
class OTAFilterTPTNewtonRaphson
    : public ParameterListenerManager<OTAFilterTPTNewtonRaphson, FilterParamId> {
 public:
  OTAFilterTPTNewtonRaphson(juce::AudioProcessorValueTreeState& apvts,
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
  float ProcessSample(float in, float env_sample, float lfo_sample);

  // Evaluate filter for a given output guess.
  // Returns what the output would be if the actual output were 'out_guess'
  float EvaluateFilter(float in, float out_guess, float G, float k,
    float& v1_out, float& v2_out, float& v3_out, float& v4_out, bool use_adaa = false) const;
  // Compute derivative for newton raphson
  // d(output)/d(out_guess) = how much does changing our guess change the predicted output?
  float ComputeJacobian(float in, float out_guess, float G, float k) const;

  const juce::AudioBuffer<float>& env1_buffer_;
  const juce::AudioBuffer<float>& env2_buffer_;
  bool use_env1_ = true;
  const juce::AudioBuffer<float>& lfo_buffer_;
  float sample_rate_;
  // state vars for each stage
  float s1_, s2_, s3_, s4_;
  // Tanh ADAA for each stage's input
  // todo: should these really have mutable keyword?
  mutable std::array<TanhADAA, 4> tanh_stages_;
  // Tanh ADAA for each stage's state
  mutable std::array<TanhADAA, 4> state_tanh_stages_;
};

}  // namespace audio_plugin