#include "Env1Section.h"

namespace audio_plugin {

Env1Section::Env1Section(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  env1_label_.setText("ENV1", juce::dontSendNotification);
  addAndMakeVisible(env1_label_);
  env1_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_attack_slider_);
  env1_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "adsrAttack", env1_attack_slider_);
  env1_attack_label_.setText("A", juce::dontSendNotification);
  addAndMakeVisible(env1_attack_label_);

  env1_decay_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_decay_slider_);
  env1_decay_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "adsrDecay", env1_decay_slider_);
  env1_decay_label_.setText("D", juce::dontSendNotification);
  addAndMakeVisible(env1_decay_label_);

  env1_sustain_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_sustain_slider_);
  env1_sustain_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "adsrSustain", env1_sustain_slider_);
  env1_sustain_label_.setText("S", juce::dontSendNotification);
  addAndMakeVisible(env1_sustain_label_);

  env1_release_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_release_slider_);
  env1_release_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "adsrRelease", env1_release_slider_);
  env1_release_label_.setText("R", juce::dontSendNotification);
  addAndMakeVisible(env1_release_label_);
}

void Env1Section::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{env1_attack_slider_},
                        juce::GridItem{env1_decay_slider_},
                        juce::GridItem{env1_sustain_slider_},
                        juce::GridItem{env1_release_slider_},
                        juce::GridItem{env1_attack_label_},
                        juce::GridItem{env1_decay_label_},
                        juce::GridItem{env1_sustain_label_},
                        juce::GridItem{env1_release_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());
}

}  // namespace audio_plugin
