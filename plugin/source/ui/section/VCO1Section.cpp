import JuceImports;
import std;

#include "VCO1Section.h"

namespace audio_plugin {

VCO1Section::VCO1Section(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vco1_label_.setText("VCO 1", juce::dontSendNotification);
  addAndMakeVisible(vco1_label_);

  // Wave type selectors
  const juce::StringArray waveTypeOptions = {"SIN", "SAW", "TRI", "SQR", "RND"};
  for (int i = 0; i < waveTypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(waveTypeOptions[i]);
    btn->setRadioGroupId(1001);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("waveType");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    wave_type_buttons_.emplace_back(std::move(btn));
  }
  wave_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("waveType"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        wave_type_buttons_[index]->setToggleState(true,
                                                  juce::dontSendNotification);
      });
  wave_type_attachment_->sendInitialUpdate();

  for (auto& btn : wave_type_buttons_) {
    addAndMakeVisible(*btn);
  }

  wave_type_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(wave_type_label_);

  vco1_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco1_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vco1_level_slider_);
  vco1_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vco1Level", vco1_level_slider_);
  vco1_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vco1_level_label_);
}

void VCO1Section::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};

  section_grid.items = {juce::GridItem{vco1_label_}.withArea(1, 1, 1, 3),
                        juce::GridItem{},  // placeholder for radio area
                        juce::GridItem{vco1_level_slider_},
                        juce::GridItem{wave_type_label_},
                        juce::GridItem{vco1_level_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto radio_area = section_grid.items[1].currentBounds.toNearestInt();
  const auto button_height = radio_area.getHeight() / 10;

  for (auto& btn : wave_type_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}

}  // namespace audio_plugin
