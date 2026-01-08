#include "LFOSection.h"

namespace audio_plugin {

LFOSection::LFOSection(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  lfo_label_.setText("LFO", juce::dontSendNotification);
  addAndMakeVisible(lfo_label_);

  rate_label_.setText("Rate", juce::dontSendNotification);
  addAndMakeVisible(rate_label_);

  rate_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  rate_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(rate_slider_);
  rate_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "lfoRate", rate_slider_);

  delay_time_label_.setText("Delay Time", juce::dontSendNotification);
  addAndMakeVisible(delay_time_label_);

  delay_time_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  delay_time_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(delay_time_slider_);
  delay_time_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "lfoDelayTimeSeconds", delay_time_slider_);

  lfo_attack_label_.setText("Attack", juce::dontSendNotification);
  addAndMakeVisible(lfo_attack_label_);

  lfo_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  lfo_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(lfo_attack_slider_);
  lfo_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "lfoAttack", lfo_attack_slider_);

  lfo_wave_form_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(lfo_wave_form_label_);


  const juce::StringArray lfoWaveTypeOptions = {"SIN", "SAW", "TRI", "SQR",
                                                "RND"};
  for (int i = 0; i < lfoWaveTypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(lfoWaveTypeOptions[i]);
    btn->setRadioGroupId(1002);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processor_.apvts_.getParameter("lfoWaveType");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    lfo_wave_type_buttons_.emplace_back(std::move(btn));
  }
  lfo_wave_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processor_.apvts_.getParameter("lfoWaveType"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        lfo_wave_type_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  lfo_wave_type_attachment_->sendInitialUpdate();

  for (auto& btn : lfo_wave_type_buttons_) {
    addAndMakeVisible(*btn);
  }
}
void LFOSection::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(3))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{lfo_label_},
                        juce::GridItem{},
                        juce::GridItem{},
                        juce::GridItem{},
                        juce::GridItem{rate_slider_},
                        juce::GridItem{delay_time_slider_},
                        juce::GridItem{lfo_attack_slider_},
                        juce::GridItem{},
                        juce::GridItem{rate_label_},
                        juce::GridItem{delay_time_label_},
                        juce::GridItem{lfo_attack_label_},
                        juce::GridItem{lfo_wave_form_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto radio_area = section_grid.items[7].currentBounds.toNearestInt();
  const auto button_height = radio_area.getHeight() / 10;

  for (const auto& btn : lfo_wave_type_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}
}  // namespace audio_plugin