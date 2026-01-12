#pragma once

import JuceImports;
import std;

#include "../../PluginProcessor.h"

namespace audio_plugin {

class LFOSection : public juce::Component {
 public:
  explicit LFOSection(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;
  juce::Label lfo_label_;
  // rate
  juce::Label rate_label_;
  juce::Slider rate_slider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      rate_attachment_;
  // delay time
  juce::Label delay_time_label_;
  juce::Slider delay_time_slider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delay_time_attachment_;
  // attack
  juce::Label lfo_attack_label_;
  juce::Slider lfo_attack_slider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      lfo_attack_attachment_;
  // wave form
  std::vector<std::unique_ptr<juce::ToggleButton>> lfo_wave_type_buttons_;
  juce::Label lfo_wave_form_label_;
  std::unique_ptr<juce::ParameterAttachment> lfo_wave_type_attachment_;
};

}  // namespace audio_plugin
