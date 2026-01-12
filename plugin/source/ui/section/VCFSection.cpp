import JuceImports;
import std;

#include "VCFSection.h"

namespace audio_plugin {

VCFSection::VCFSection(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vcf_label_.setText("VCF", juce::dontSendNotification);
  addAndMakeVisible(vcf_label_);

  filter_hpf_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_hpf_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(filter_hpf_slider_);
  filter_hpf_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "hpfFreq", filter_hpf_slider_);

  filter_hpf_label_.setText("HPF", juce::dontSendNotification);
  addAndMakeVisible(filter_hpf_label_);

  filter_cutoff_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_cutoff_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                        20);
  addAndMakeVisible(filter_cutoff_slider_);
  filter_cutoff_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "filterCutoffFreq", filter_cutoff_slider_);

  filter_cutoff_label_.setText("Cutoff", juce::dontSendNotification);
  addAndMakeVisible(filter_cutoff_label_);

  filter_resonance_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_resonance_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                           80, 20);
  addAndMakeVisible(filter_resonance_slider_);
  filter_resonance_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "filterResonance", filter_resonance_slider_);

  filter_resonance_label_.setText("Resonance", juce::dontSendNotification);
  addAndMakeVisible(filter_resonance_label_);

  filter_drive_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_drive_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(filter_drive_slider_);
  filter_drive_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "filterDrive", filter_drive_slider_);

  filter_drive_label_.setText("Drive", juce::dontSendNotification);
  addAndMakeVisible(filter_drive_label_);

  const juce::StringArray filterSlopeOptions = {"-24", "-18", "-12"};
  for (int i = 0; i < filterSlopeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(filterSlopeOptions[i]);
    btn->setRadioGroupId(1005);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("filterSlope");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    filter_slope_buttons_.emplace_back(std::move(btn));
  }
  filter_slope_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("filterSlope"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        filter_slope_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  filter_slope_attachment_->sendInitialUpdate();

  for (auto& btn : filter_slope_buttons_) {
    addAndMakeVisible(*btn);
  }
  filter_slope_label_.setText("Slope", juce::dontSendNotification);
  addAndMakeVisible(filter_slope_label_);

  filter_env_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_env_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                         20);
  addAndMakeVisible(filter_env_mod_slider_);
  filter_env_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "filterEnvMod", filter_env_mod_slider_);
  filter_env_mod_label_.setText("Env Mod", juce::dontSendNotification);
  addAndMakeVisible(filter_env_mod_label_);

  filter_lfo_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_lfo_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                         20);
  addAndMakeVisible(filter_lfo_mod_slider_);
  filter_lfo_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "filterLfoMod", filter_lfo_mod_slider_);
  filter_lfo_mod_label_.setText("LFO Mod", juce::dontSendNotification);
  addAndMakeVisible(filter_lfo_mod_label_);

  const juce::StringArray filterEnvSourceOptions = {"E1", "E2"};
  for (int i = 0; i < filterEnvSourceOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(filterEnvSourceOptions[i]);
    btn->setRadioGroupId(1006);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("filterEnvSource");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    filter_env_source_buttons_.emplace_back(std::move(btn));
  }
  filter_env_source_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("filterEnvSource"),
      [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        filter_env_source_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  filter_env_source_attachment_->sendInitialUpdate();

  for (auto& btn : filter_env_source_buttons_) {
    addAndMakeVisible(*btn);
  }
  filter_env_source_label_.setText("Env Source", juce::dontSendNotification);
  addAndMakeVisible(filter_env_source_label_);

  filter_type_combo_.clear(juce::dontSendNotification);
  filter_type_combo_.addItemList(
      {"Delayed Feedback", "TPT Newton-Raphson", "Disabled"}, 1);
  addAndMakeVisible(filter_type_combo_);
  filter_type_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor_.apvts_, "vcfFilterType", filter_type_combo_);
}

void VCFSection::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{vcf_label_}.withArea(1, 1, 1, 3),
                        juce::GridItem{filter_type_combo_}.withArea(1, 3, 1, 9),
                        juce::GridItem{filter_hpf_slider_},
                        juce::GridItem{filter_cutoff_slider_},
                        juce::GridItem{filter_resonance_slider_},
                        juce::GridItem{filter_drive_slider_},
                        juce::GridItem{},
                        juce::GridItem{filter_env_mod_slider_},
                        juce::GridItem{filter_lfo_mod_slider_},
                        juce::GridItem{},
                        juce::GridItem{filter_hpf_label_},
                        juce::GridItem{filter_cutoff_label_},
                        juce::GridItem{filter_resonance_label_},
                        juce::GridItem{filter_drive_label_},
                        juce::GridItem{filter_slope_label_},
                        juce::GridItem{filter_env_mod_label_},
                        juce::GridItem{filter_lfo_mod_label_},
                        juce::GridItem{filter_env_source_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  // filter slope layout
  {
    auto radio_area = section_grid.items[6].currentBounds.toNearestInt();
    const auto button_height =
        radio_area.getHeight() / static_cast<int>(filter_slope_buttons_.size());

    for (auto& btn : filter_slope_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }

  // filter env source layout
  {
    auto radio_area = section_grid.items[9].currentBounds.toNearestInt();
    const auto button_height =
        radio_area.getHeight() /
        static_cast<int>(filter_env_source_buttons_.size());

    for (auto& btn : filter_env_source_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }
}

}  // namespace audio_plugin
