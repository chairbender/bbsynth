#pragma once

#include "WaveGenerator.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "OTAFilter.h"

namespace audio_plugin {
struct OscillatorSound : juce::SynthesiserSound {
  OscillatorSound(juce::AudioProcessorValueTreeState& apvts);

  bool appliesToNote([[maybe_unused]] int midiNoteNumber) override;
  bool appliesToChannel([[maybe_unused]] int midiChannelNumber) override;
  std::atomic<float>* getCentOffset() const;

private:
  std::atomic<float>* centOffset_;
};

struct OscillatorVoice : juce::SynthesiserVoice {
  OscillatorVoice();

  bool canPlaySound(juce::SynthesiserSound* sound) override;

  /**
   * Update parameters based on current state.
   * Typically should be called at start of each block.
   */
  void Configure(const juce::AudioProcessorValueTreeState& apvts);

  void startNote(int midiNoteNumber,
                 float velocity,
                 [[maybe_unused]] juce::SynthesiserSound* sound,
                 [[maybe_unused]] int pitchWheelPos) override;

  void stopNote([[maybe_unused]] float velocity, bool allowTailOff) override;

  void pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) override;
  void controllerMoved([[maybe_unused]] int controllerNumber,
                       [[maybe_unused]] int newControllerValue) override;

  void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                       int startSample,
                       int numSamples) override;

  // Test-only accessor to inspect internal generator state
  // todo below comment needed?
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  WaveGenerator& getWaveGeneratorForTest() { return waveGenerator_; }

private:
  WaveGenerator waveGenerator_;
  OTAFilter filter_;
  juce::AudioBuffer<float> oversample_buffer_;
  // note we only use this for the downsampling - internally
  // we generate the oscillator already oversampled
  juce::dsp::Oversampling<float> oversampler_;
};
}