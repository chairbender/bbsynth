#pragma once

#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCO1Section : public juce::Component {
 public:
  explicit VCO1Section(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label vco1_label_;
  std::vector<std::unique_ptr<juce::ToggleButton>> wave_type_buttons_;
  std::unique_ptr<juce::ParameterAttachment> wave_type_attachment_;
  juce::Label wave_type_label_;

  juce::Slider vco1_level_slider_;
  juce::Label vco1_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vco1_level_attachment_;
};

}  // namespace audio_plugin
