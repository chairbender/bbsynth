#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "AnalogADSR.h"
#include "Downsampler.h"
#include "OTAFilter.h"
#include "WaveGenerator.h"

namespace audio_plugin {
struct OscillatorSound : juce::SynthesiserSound {
  OscillatorSound(juce::AudioProcessorValueTreeState& apvts);

  bool appliesToNote([[maybe_unused]] int midiNoteNumber) override;
  bool appliesToChannel([[maybe_unused]] int midiChannelNumber) override;
};

struct OscillatorVoice : juce::SynthesiserVoice {
  OscillatorVoice(const juce::AudioBuffer<float>& lfo_buffer);
  bool canPlaySound(juce::SynthesiserSound* sound) override;

  /**
   * Update parameters based on current state.
   * Typically should be called at start of each block.
   */
  void Configure(const juce::AudioProcessorValueTreeState& apvts);

  /**
   *
   * @param blockSize Number of samples to expect per buffer (needed for
   * oversampler)
   */
  void SetBlockSize(int blockSize);

  void startNote(int midiNoteNumber, float velocity,
                 [[maybe_unused]] juce::SynthesiserSound* sound,
                 [[maybe_unused]] int pitchWheelPos) override;

  void stopNote([[maybe_unused]] float velocity, bool allowTailOff) override;

  void pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) override;
  void controllerMoved([[maybe_unused]] int controllerNumber,
                       [[maybe_unused]] int newControllerValue) override;

  void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample,
                       int numSamples) override;

  // Test-only accessor to inspect internal generator state
  // todo below comment needed?
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  WaveGenerator<false>& getWaveGeneratorForTest() { return waveGenerator_; }

 private:
  juce::AudioBuffer<float> env1_buffer_;
  juce::AudioBuffer<float> env2_buffer_;
  const juce::AudioBuffer<float>& lfo_buffer_;
  // sub-sample accurate sample indices for the current block of when the
  // secondary's resets should occur.
  // Be warned - This can contain a negative value
  // if the reset occurred between the last sample of the previous block and
  // the first sample of the current block.
  // todo: do we actually need CriticalSection?
  //   I don't think it's written concurrently...
  juce::Array<float> hard_sync_reset_sample_indices_;
  // modulator buffer
  // todo do we actually need this if we already hae oversample buffer?
  juce::AudioBuffer<float> wave2_buffer_;
  juce::AudioBuffer<float> oversample_buffer_;
  WaveGenerator<false> waveGenerator_;
  WaveGenerator<false> wave2Generator_;
  OTAFilter filter_;
  const juce::AudioBuffer<float>* filter_env_buffer_ = nullptr;
  Downsampler downsampler_;
  AnalogADSR envelope_;
  AnalogADSR envelope2_;
};
}  // namespace audio_plugin