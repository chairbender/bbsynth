#pragma once

#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCASection : public juce::Component {
 public:
  explicit VCASection(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label vca_label_;
  juce::Slider vca_level_slider_;
  juce::Label vca_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vca_level_attachment_;
  juce::Slider vca_lfo_mod_slider_;
  juce::Label vca_lfo_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vca_lfo_mod_attachment_;
  juce::Slider vca_tone_slider_;
  juce::Label vca_tone_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      vca_tone_attachment_;
};

}  // namespace audio_plugin
