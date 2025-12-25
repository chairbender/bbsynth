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

  //VCO 1 section
  juce::Label vco1_label_;

  // Wave selector
  juce::ComboBox wave_type_combo_;
  juce::Label wave_type_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave_type_attachment_;

  //VCO 2 section
  juce::Label vco2_label_;

  // Wave selector
  juce::ComboBox wave2_type_combo_;
  juce::Label wave2_type_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave2_type_attachment_;

  //fine tune
  juce::Slider fine_tune_slider_;
  juce::Label fine_tune_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fine_tune_attachment_;

  // VCF section
  juce::Label vcf_label_;

  juce::Slider filter_cutoff_slider_;
  juce::Label filter_cutoff_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_cutoff_attachment_;

  juce::Slider filter_resonance_slider_;
  juce::Label filter_resonance_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_resonance_attachment_;

  juce::Slider filter_drive_slider_;
  juce::Label filter_drive_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_drive_attachment_;

  // ENV1 section
  juce::Label env1_label_;
  juce::Slider env1_attack_slider_;
  juce::Label env1_attack_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_attack_attachment_;

  juce::Slider env1_decay_slider_;
  juce::Label env1_decay_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_decay_attachment_;

  juce::Slider env1_sustain_slider_;
  juce::Label env1_sustain_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_sustain_attachment_;

  juce::Slider env1_release_slider_;
  juce::Label env1_release_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_release_attachment_;

  SpectrumAnalyzerComponent spectrum_analyzer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
}  // namespace audio_plugin
