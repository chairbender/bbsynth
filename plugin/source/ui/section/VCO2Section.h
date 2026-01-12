#pragma once

import JuceImports;
import std;

#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCO2Section : public juce::Component {
 public:
  explicit VCO2Section(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label vco2_label_;
  std::vector<std::unique_ptr<juce::ToggleButton>> wave2_type_buttons_;
  juce::Label wave2_type_label_;
  std::unique_ptr<juce::ParameterAttachment> wave2_type_attachment_;

  juce::Slider vco2_level_slider_;
  juce::Label vco2_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vco2_level_attachment_;

  juce::Slider cross_mod_slider_;
  juce::Label cross_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      cross_mod_attachment_;

  juce::Slider fine_tune_slider_;
  juce::Label fine_tune_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      fine_tune_attachment_;

  juce::ToggleButton vco2_sync_button_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      vco2_sync_attachment_;
};

}  // namespace audio_plugin
