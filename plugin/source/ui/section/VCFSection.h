#pragma once

#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCFSection : public juce::Component {
 public:
  explicit VCFSection(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label vcf_label_;
  juce::Slider filter_hpf_slider_;
  juce::Label filter_hpf_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_hpf_attachment_;
  juce::Slider filter_cutoff_slider_;
  juce::Label filter_cutoff_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_cutoff_attachment_;

  juce::Slider filter_resonance_slider_;
  juce::Label filter_resonance_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_resonance_attachment_;

  juce::Slider filter_drive_slider_;
  juce::Label filter_drive_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_drive_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> filter_slope_buttons_;
  juce::Label filter_slope_label_;
  std::unique_ptr<juce::ParameterAttachment> filter_slope_attachment_;

  juce::Slider filter_env_mod_slider_;
  juce::Label filter_env_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_env_mod_attachment_;

  juce::Slider filter_lfo_mod_slider_;
  juce::Label filter_lfo_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      filter_lfo_mod_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> filter_env_source_buttons_;
  juce::Label filter_env_source_label_;
  std::unique_ptr<juce::ParameterAttachment> filter_env_source_attachment_;

  juce::ComboBox filter_type_combo_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      filter_type_attachment_;
};

}  // namespace audio_plugin
