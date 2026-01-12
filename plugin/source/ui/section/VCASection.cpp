import JuceImports;
import std;

#include "VCASection.h"

namespace audio_plugin {

VCASection::VCASection(AudioPluginAudioProcessor& processor)
    : processor_{processor} {
  vca_label_.setText("VCA", juce::dontSendNotification);
  addAndMakeVisible(vca_label_);

  vca_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vca_level_slider_);
  vca_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vcaLevel", vca_level_slider_);
  vca_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vca_level_label_);

  vca_lfo_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_lfo_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vca_lfo_mod_slider_);
  vca_lfo_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vcaLfoMod", vca_lfo_mod_slider_);
  vca_lfo_mod_label_.setText("LFO Mod", juce::dontSendNotification);
  addAndMakeVisible(vca_lfo_mod_label_);

  vca_tone_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_tone_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vca_tone_slider_);
  vca_tone_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor_.apvts_, "vcaTone", vca_tone_slider_);
  vca_tone_label_.setText("Tone", juce::dontSendNotification);
  addAndMakeVisible(vca_tone_label_);
}

void VCASection::resized() {
  const auto section_bounds = getLocalBounds().reduced(4);
  juce::Grid section_grid;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {
      juce::GridItem{vca_label_}.withArea(1, 1, 1, 4),
      juce::GridItem{vca_level_slider_},
      juce::GridItem{vca_lfo_mod_slider_},
      juce::GridItem{vca_tone_slider_},
      juce::GridItem{vca_level_label_},
      juce::GridItem{vca_lfo_mod_label_},
      juce::GridItem{vca_tone_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());
}

}  // namespace audio_plugin
