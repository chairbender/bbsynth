#pragma once
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {
/**
 * The built-in juce ADSR is linear. For a more authentic analog ADSR,
 * we want it to curve. That's what this does.
 */
class AnalogADSR {
  public:
  void Prepare(double sample_rate);

  void Configure(float attack_seconds, float decay_seconds, float sustain_level, float release_seconds);

  void Reset();
  void NoteOn();
  void NoteOff();
  // this does NOT APPLY the envelope to the buffer - it writes the raw envelope
  // values to the buffer so the envelope can be used by other parts of the plugin
  // This only affects the first channel of the buffer.
  void WriteEnvelopeToBuffer(juce::AudioBuffer<float>& buffer, int start_sample, int num_samples);
  bool IsActive() const;

 private:
  void AdvanceStateFromAttack();
  void AdvanceStateFromDecay();
  void AdvanceStateFromSustain();
  void AdvanceStateFromRelease();

  template<auto NextStateFunc,auto Curve,auto ConfiguredStageSamples,auto StartVal, auto TargetVal>
  void WriteStage(juce::AudioBuffer<float>& buffer, int start_sample,
                  int num_samples);

  enum class State { idle, attack, decay, sustain, release };
  State state_{State::idle};
  float sample_rate_{0};

  // configured amount of samples desired per stage
  int attack_samples_{0};
  int decay_samples_{0};
  int release_samples_{0};
  float sustain_level_{0.5};

  // how many samples has the current stage lasted (only relevant for attack, decay, release)
  int stage_samples_{0};
  // what was the level when the release started?
  float released_level_{0.f};
  // most recent output level, used for setting released level
  float last_level_{0.f};
};
}  // namespace audio_plugin
