#pragma once

import JuceImports;
import std;

#include "../../PluginProcessor.h"

namespace audio_plugin {

class VCFDriveScalingSection : public juce::Component {
 public:
  explicit VCFDriveScalingSection(AudioPluginAudioProcessor& processor);

  void resized() override;

 private:
  AudioPluginAudioProcessor& processor_;

  juce::Label vcf_drive_scaling_label_;
  std::array<juce::Slider, 4> filter_input_drive_scale_sliders_;
  std::array<juce::Slider, 4> filter_state_drive_scale_sliders_;
  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
             4>
      filter_input_drive_scale_attachments_;
  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
             4>
      filter_state_drive_scale_attachments_;

  // Headers for the drive scaling table
  std::array<juce::Label, 4> filter_stage_header_labels_;
  juce::Label filter_input_row_label_;
  juce::Label filter_state_row_label_;
};

}  // namespace audio_plugin
