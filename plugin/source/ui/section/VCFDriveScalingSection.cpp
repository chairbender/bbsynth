import JuceImports;
import std;

#include "VCFDriveScalingSection.h"

namespace audio_plugin {

VCFDriveScalingSection::VCFDriveScalingSection(
    AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vcf_drive_scaling_label_.setText("VCF Drive Scaling",
                                   juce::dontSendNotification);
  vcf_drive_scaling_label_.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  vcf_drive_scaling_label_.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(vcf_drive_scaling_label_);

  for (int i = 0; i < 4; ++i) {
    auto& inSlider = filter_input_drive_scale_sliders_[static_cast<size_t>(i)];
    inSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    inSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(inSlider);
    filter_input_drive_scale_attachments_[static_cast<size_t>(i)] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.apvts_, "filterInputDriveScale" + juce::String(i + 1),
            inSlider);

    auto& stateSlider =
        filter_state_drive_scale_sliders_[static_cast<size_t>(i)];
    stateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(stateSlider);
    filter_state_drive_scale_attachments_[static_cast<size_t>(i)] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.apvts_, "filterStateDriveScale" + juce::String(i + 1),
            stateSlider);

    auto& stageLabel = filter_stage_header_labels_[static_cast<size_t>(i)];
    stageLabel.setText(juce::String(i + 1), juce::dontSendNotification);
    stageLabel.setJustificationType(juce::Justification::centred);
    stageLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(stageLabel);
  }

  filter_input_row_label_.setText("Input", juce::dontSendNotification);
  filter_input_row_label_.setJustificationType(
      juce::Justification::centredRight);
  addAndMakeVisible(filter_input_row_label_);

  filter_state_row_label_.setText("State", juce::dontSendNotification);
  filter_state_row_label_.setJustificationType(
      juce::Justification::centredRight);
  addAndMakeVisible(filter_state_row_label_);
}

void VCFDriveScalingSection::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  // 5 columns: 1 for row labels, 4 for stages
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2))};
  // 4 rows: 1 for section label, 1 for headers, 1 for input, 1 for state
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4))};
  section_grid.items = {
      // Row 0
      juce::GridItem{vcf_drive_scaling_label_}.withArea(1, 1, 1, 6),
      // Row 1
      juce::GridItem{}, juce::GridItem{filter_stage_header_labels_[0]},
      juce::GridItem{filter_stage_header_labels_[1]},
      juce::GridItem{filter_stage_header_labels_[2]},
      juce::GridItem{filter_stage_header_labels_[3]},
      // Row 2
      juce::GridItem{filter_input_row_label_},
      juce::GridItem{filter_input_drive_scale_sliders_[0]},
      juce::GridItem{filter_input_drive_scale_sliders_[1]},
      juce::GridItem{filter_input_drive_scale_sliders_[2]},
      juce::GridItem{filter_input_drive_scale_sliders_[3]},
      // Row 3
      juce::GridItem{filter_state_row_label_},
      juce::GridItem{filter_state_drive_scale_sliders_[0]},
      juce::GridItem{filter_state_drive_scale_sliders_[1]},
      juce::GridItem{filter_state_drive_scale_sliders_[2]},
      juce::GridItem{filter_state_drive_scale_sliders_[3]}};

  section_grid.performLayout(section_bounds.toNearestInt());
}

}  // namespace audio_plugin
