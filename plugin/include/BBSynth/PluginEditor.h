#pragma once

#include "PluginProcessor.h"
#include "SpectrumAnalyzerComponent.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
  /**
   * must be called by root component to update audio-reactive components
   */
  void GetNextAudioBlock(juce::AudioBuffer<float>& buffer);
  ~AudioPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  // todo refactor so this can be private
  juce::MidiKeyboardState keyboard_state_;


private:
  juce::MidiKeyboardComponent keyboardComponent;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  juce::Label cent_offset_label_;
  juce::Slider cent_offset_slider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cent_offset_attachment_;

  juce::Slider filter_cutoff_slider_;
  juce::Label filter_cutoff_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_cutoff_attachment_;

  juce::Slider filter_resonance_slider_;
  juce::Label filter_resonance_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_resonance_attachment_;

  juce::Slider filter_drive_slider_;
  juce::Label filter_drive_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_drive_attachment_;

  juce::ToggleButton filter_enabled_button_;
  juce::Label filter_enabled_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filter_enabled_attachment_;

  SpectrumAnalyzerComponent spectrum_analyzer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
}  // namespace audio_plugin
