#include "Env2Section.h"

namespace audio_plugin {

Env2Section::Env2Section(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  env2_label_.setText("ENV2", juce::dontSendNotification);
  addAndMakeVisible(env2_label_);
  env2_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env2_attack_slider_);
  env2_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "env2Attack", env2_attack_slider_);
  env2_attack_label_.setText("A", juce::dontSendNotification);
  addAndMakeVisible(env2_attack_label_);

  env2_decay_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env2_decay_slider_);
  env2_decay_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "env2Decay", env2_decay_slider_);
  env2_decay_label_.setText("D", juce::dontSendNotification);
  addAndMakeVisible(env2_decay_label_);

  env2_sustain_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_sustain_slider_);
  env2_sustain_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "env2Sustain", env2_sustain_slider_);
  env2_sustain_label_.setText("S", juce::dontSendNotification);
  addAndMakeVisible(env2_sustain_label_);

  env2_release_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_release_slider_);
  env2_release_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "env2Release", env2_release_slider_);
  env2_release_label_.setText("R", juce::dontSendNotification);
  addAndMakeVisible(env2_release_label_);
}

void Env2Section::resized() {
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
  section_grid.items = {juce::GridItem{env2_label_}.withArea(1, 1, 1, 5),
                        juce::GridItem{env2_attack_slider_},
                        juce::GridItem{env2_decay_slider_},
                        juce::GridItem{env2_sustain_slider_},
                        juce::GridItem{env2_release_slider_},
                        juce::GridItem{env2_attack_label_},
                        juce::GridItem{env2_decay_label_},
                        juce::GridItem{env2_sustain_label_},
                        juce::GridItem{env2_release_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());
}

}  // namespace audio_plugin
