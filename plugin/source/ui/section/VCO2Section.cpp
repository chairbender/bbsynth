#include "VCO2Section.h"

namespace audio_plugin {

VCO2Section::VCO2Section(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vco2_label_.setText("VCO 2", juce::dontSendNotification);
  addAndMakeVisible(vco2_label_);

  // Wave type selector
  const juce::StringArray wave2TypeOptions = {"SIN", "SAW", "TRI", "SQR", "RND"};
  for (int i = 0; i < wave2TypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(wave2TypeOptions[i]);
    btn->setRadioGroupId(1003);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("wave2Type");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    wave2_type_buttons_.emplace_back(std::move(btn));
  }
  wave2_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("wave2Type"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        wave2_type_buttons_[index]->setToggleState(true,
                                                   juce::dontSendNotification);
      });
  wave2_type_attachment_->sendInitialUpdate();

  for (auto& btn : wave2_type_buttons_) {
    addAndMakeVisible(*btn);
  }

  wave2_type_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(wave2_type_label_);

  vco2_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco2_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vco2_level_slider_);
  vco2_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vco2Level", vco2_level_slider_);
  vco2_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vco2_level_label_);

  cross_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  cross_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(cross_mod_slider_);
  cross_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "crossMod", cross_mod_slider_);
  cross_mod_label_.setText("Cross Mod", juce::dontSendNotification);
  addAndMakeVisible(cross_mod_label_);

  // fine tune
  fine_tune_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  fine_tune_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  fine_tune_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "fineTune", fine_tune_slider_);
  fine_tune_label_.setText("Fine Tune", juce::dontSendNotification);
  addAndMakeVisible(fine_tune_label_);
  addAndMakeVisible(fine_tune_slider_);

  // sync
  vco2_sync_button_.setButtonText("Sync");
  vco2_sync_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco2_sync_button_);
  vco2_sync_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor_.apvts_, "vco2Sync", vco2_sync_button_);
}

void VCO2Section::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {
      juce::GridItem{vco2_label_}.withArea(1, 1, 1, 3),
      juce::GridItem{vco2_sync_button_}.withArea(1, 3, 1, 5),
      juce::GridItem{cross_mod_slider_},
      juce::GridItem{},
      juce::GridItem{fine_tune_slider_},
      juce::GridItem{vco2_level_slider_},
      juce::GridItem{cross_mod_label_},
      juce::GridItem{wave2_type_label_},
      juce::GridItem{fine_tune_label_},
      juce::GridItem{vco2_level_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto radio_area = section_grid.items[3].currentBounds.toNearestInt();
  const auto button_height = radio_area.getHeight() / 10;

  for (auto& btn : wave2_type_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}

}  // namespace audio_plugin
