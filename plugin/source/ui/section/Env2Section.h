#pragma once

#include "../../PluginProcessor.h"

namespace audio_plugin {

class Env2Section : public juce::Component {
 public:
  explicit Env2Section(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label env2_label_;
  juce::Slider env2_attack_slider_;
  juce::Label env2_attack_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env2_attack_attachment_;

  juce::Slider env2_decay_slider_;
  juce::Label env2_decay_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env2_decay_attachment_;

  juce::Slider env2_sustain_slider_;
  juce::Label env2_sustain_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env2_sustain_attachment_;

  juce::Slider env2_release_slider_;
  juce::Label env2_release_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env2_release_attachment_;
};

}  // namespace audio_plugin
