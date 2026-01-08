#include "VCOModSection.h"

namespace audio_plugin {
VCOModSection::VCOModSection(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vco_mod_label_.setText("VCO Modulator", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_label_);

  // lfo -> freq mod
  vco_mod_lfo_freq_label_.setText("LFO Freq Mod", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_lfo_freq_label_);
  vco_mod_lfo_freq_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco_mod_lfo_freq_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                           50, 20);
  addAndMakeVisible(vco_mod_lfo_freq_slider_);
  vco_mod_freq_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vcoModLfoFreq", vco_mod_lfo_freq_slider_);

  // env1 -> freq mod
  vco_mod_env1_freq_label_.setText("ENV1 Freq Mod", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_env1_freq_label_);
  vco_mod_env1_freq_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco_mod_env1_freq_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                            50, 20);
  addAndMakeVisible(vco_mod_env1_freq_slider_);
  vco_mod_env1_freq_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vcoModEnv1Freq", vco_mod_env1_freq_slider_);

  vco_mod_osc1_button_.setButtonText("VCO 1");
  vco_mod_osc1_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc1_button_);
  vco_mod_osc1_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor_.apvts_, "vcoModOsc1", vco_mod_osc1_button_);

  vco_mod_osc2_button_.setButtonText("VCO 2");
  vco_mod_osc2_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc2_button_);
  vco_mod_osc2_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor_.apvts_, "vcoModOsc2", vco_mod_osc2_button_);

  pulse_width_label_.setText("Pulse Width", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_label_);
  pulse_width_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  pulse_width_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                      20);
  addAndMakeVisible(pulse_width_slider_);
  pulse_width_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "pulseWidth", pulse_width_slider_);

  const juce::StringArray pwSourceOptions = {"E2-", "E2+", "E1-",
                                             "E1+", "LFO", "MAN"};
  for (int i = 0; i < pwSourceOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(pwSourceOptions[i]);
    btn->setRadioGroupId(1004);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("pulseWidthSource");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    pulse_width_source_buttons_.emplace_back(std::move(btn));
  }
  pulse_width_source_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("pulseWidthSource"),
      [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        pulse_width_source_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  pulse_width_source_attachment_->sendInitialUpdate();
  pulse_width_source_label_.setText("PW Source", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_source_label_);
  for (auto& btn : pulse_width_source_buttons_) {
    addAndMakeVisible(*btn);
  }
}
void VCOModSection::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(4))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{vco_mod_label_},
                        juce::GridItem{},
                        juce::GridItem{},
                        juce::GridItem{},
                        juce::GridItem{},
                        juce::GridItem{vco_mod_lfo_freq_slider_},
                        juce::GridItem{vco_mod_env1_freq_slider_},
                        juce::GridItem{},
                        juce::GridItem{pulse_width_slider_},
                        juce::GridItem{},
                        juce::GridItem{vco_mod_lfo_freq_label_},
                        juce::GridItem{vco_mod_env1_freq_label_},
                        juce::GridItem{},
                        juce::GridItem{pulse_width_label_},
                        juce::GridItem{pulse_width_source_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto button_area = section_grid.items[7].currentBounds;
  button_area.removeFromBottom(button_area.getHeight() / 2);
  vco_mod_osc1_button_.setBounds(
      button_area.removeFromTop(button_area.getHeight() / 2).toNearestInt());
  vco_mod_osc2_button_.setBounds(button_area.toNearestInt());

  auto radio_area = section_grid.items[9].currentBounds.toNearestInt();
  const auto button_height =
      radio_area.getHeight() / 2 /
      static_cast<int>(pulse_width_source_buttons_.size());

  for (const auto& btn : pulse_width_source_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}
}  // namespace audio_plugin