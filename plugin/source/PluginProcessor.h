#pragma once
import JuceImports;
import std;

#include "filter/ToneFilter.h"
#include "oscillator/WaveGenerator.h"

namespace audio_plugin {

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
  void ConfigureLFO();

  void parameterChanged(const juce::String& name, float newValue) override;

  // todo: passing this around is a stupid way to do it. Let's find a better way...
  juce::AudioBuffer<float> lfo_buffer_;
  juce::Synthesiser synth;
  WaveGenerator<true> lfo_generator_;
  juce::dsp::IIR::Filter<float> hpf_;
  ToneFilter tone_filter_;
  juce::dsp::Limiter<float> main_limiter_;
  // how many samples remaining until LFO should start,
  // < 0  means LFO is not playing.
  int lfo_samples_until_start_;
  // short ramp up for the LFO so it starts smoothly
  // -1 means not ramping
  float lfo_ramp_;
  float lfo_ramp_step_;
  // configured delay time
  float lfo_delay_time_s_;
  // configured rate
  float lfo_rate_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
}  // namespace audio_plugin
