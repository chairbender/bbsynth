#pragma once
#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCOModSection : public juce::Component {
 public:
  explicit VCOModSection(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;
  juce::Label vco_mod_label_;
  // lfo -> freq mod
  juce::Slider vco_mod_lfo_freq_slider_;
  juce::Label vco_mod_lfo_freq_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vco_mod_freq_attachment_;
  // env1 -> freq mod
  juce::Slider vco_mod_env1_freq_slider_;
  juce::Label vco_mod_env1_freq_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vco_mod_env1_freq_attachment_;
  // VCO picker - osc1, osc2, or both
  juce::ToggleButton vco_mod_osc1_button_;
  juce::ToggleButton vco_mod_osc2_button_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      vco_mod_osc1_attachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      vco_mod_osc2_attachment_;

  // pulse width
  juce::Slider pulse_width_slider_;
  juce::Label pulse_width_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      pulse_width_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> pulse_width_source_buttons_;
  juce::Label pulse_width_source_label_;
  std::unique_ptr<juce::ParameterAttachment> pulse_width_source_attachment_;
};

}  // namespace audio_plugin