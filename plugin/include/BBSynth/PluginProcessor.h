#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "BBSynth/TanhADAA.h"

namespace audio_plugin {

struct OTAFilter {
  OTAFilter();

  // OTA filter params
  // todo refactor to separate class?
  // integrator states
  float s1, s2, s3, s4;
  std::array<TanhADAA, 4> tanh_in_;
  std::array<TanhADAA, 4> tanh_state_;
};

class AudioPluginAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener {
public:
  AudioPluginAudioProcessor();
  ~AudioPluginAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts_;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();

  void parameterChanged(const juce::String& name, float newValue) override;

  juce::Synthesiser synth;

  std::array<OTAFilter, 2> filter_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
}  // namespace audio_plugin
