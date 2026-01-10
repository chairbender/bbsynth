#pragma once

#include "../../PluginProcessor.h"

namespace audio_plugin {

class Env1Section : public juce::Component {
 public:
  explicit Env1Section(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label env1_label_;
  juce::Slider env1_attack_slider_;
  juce::Label env1_attack_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env1_attack_attachment_;

  juce::Slider env1_decay_slider_;
  juce::Label env1_decay_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env1_decay_attachment_;

  juce::Slider env1_sustain_slider_;
  juce::Label env1_sustain_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env1_sustain_attachment_;

  juce::Slider env1_release_slider_;
  juce::Label env1_release_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      env1_release_attachment_;
};

}  // namespace audio_plugin
