#include "PluginEditor.h"

#include "../PluginProcessor.h"

namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
    : AudioProcessorEditor{&p},
      keyboard_component_{keyboard_state_,
                          juce::MidiKeyboardComponent::horizontalKeyboard},
      processor_ref_(p),
      lfo_section_{p},
      vco_mod_section_{p},
      vco1_section_{p},
      vco2_section_{p},
      vcf_section_{p},
      vcf_drive_scaling_section_{p},
      vca_section_{p},
      env1_section_{p},
      env2_section_{p} {
  juce::ignoreUnused(processor_ref_);

  addAndMakeVisible(lfo_section_);
  addAndMakeVisible(vco_mod_section_);
  addAndMakeVisible(vco1_section_);
  addAndMakeVisible(vco2_section_);
  addAndMakeVisible(vcf_section_);
  addAndMakeVisible(vcf_drive_scaling_section_);
  addAndMakeVisible(vca_section_);
  addAndMakeVisible(env1_section_);
  addAndMakeVisible(env2_section_);

  setSize(1600, 900);
  centreWithSize(1600, 900);
}

void AudioPluginAudioProcessorEditor::GetNextAudioBlock(
    juce::AudioBuffer<float>& buffer) {
  spectrum_analyzer_.getNextAudioBlock(buffer);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() = default;

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  PaintBackground(g);

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  addAndMakeVisible(spectrum_analyzer_);
  addAndMakeVisible(keyboard_component_);
}

juce::Grid AudioPluginAudioProcessorEditor::LayoutMainGrid() {
  // Layout
  auto area = getLocalBounds();

  // Keep spectrum analyzer and keyboard at the bottom as-is
  keyboard_component_.setBounds(area.removeFromBottom(100));
  spectrum_analyzer_.setBounds(area.removeFromBottom(100));

  // Use a Grid to place the three sections (VCO1, VCF, ENV1) in a single row
  auto topRow = area;  // remaining area after removing bottom components

  auto grid = MakeMainGrid();

  grid.items = {
      // row 1 labels
      juce::GridItem(lfo_section_),
      juce::GridItem(vco_mod_section_),
      juce::GridItem(vco1_section_),
      juce::GridItem(vco2_section_),
      juce::GridItem(vcf_section_),
      juce::GridItem(vca_section_),
      juce::GridItem(env1_section_),
      juce::GridItem(env2_section_),
      // row 1 controls
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      // Row 2 labels
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem(vcf_drive_scaling_section_),
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      // Row 2 controls
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{}};

  grid.performLayout(topRow);
  // todo: fix after row 1+2 finally merged
  const auto row_1_height =
      grid.items[0].currentBounds.toNearestInt().getHeight();
  const auto row_2_height =
      grid.items[8].currentBounds.toNearestInt().getHeight();
  const auto combined_height = row_1_height + row_2_height;
  lfo_section_.setBounds(lfo_section_.getBounds().withHeight(combined_height));
  vco_mod_section_.setBounds(
      vco_mod_section_.getBounds().withHeight(combined_height));
  vco1_section_.setBounds(
      vco1_section_.getBounds().withHeight(combined_height));
  vco2_section_.setBounds(
      vco2_section_.getBounds().withHeight(combined_height));
  vcf_section_.setBounds(vcf_section_.getBounds().withHeight(combined_height));
  vca_section_.setBounds(vca_section_.getBounds().withHeight(combined_height));
  env1_section_.setBounds(
      env1_section_.getBounds().withHeight(combined_height));
  env2_section_.setBounds(
      env2_section_.getBounds().withHeight(combined_height));

  return grid;
}

void AudioPluginAudioProcessorEditor::resized() { LayoutMainGrid(); }

juce::Grid AudioPluginAudioProcessorEditor::MakeMainGrid() {
  juce::Grid grid;
  grid.columnGap = juce::Grid::Px(4);
  grid.rowGap = juce::Grid::Px(4);
  grid.alignContent = juce::Grid::AlignContent::center;
  grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                       juce::Grid::TrackInfo(juce::Grid::Fr(7)),
                       juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                       juce::Grid::TrackInfo(juce::Grid::Fr(7))};
  grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(3)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(3)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(5)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2))};
  return grid;
}

void AudioPluginAudioProcessorEditor::PaintBackground(juce::Graphics& g) const {
  // Layout for backgrounds
  auto area = getLocalBounds();
  // Match the area used in resized()
  area.removeFromBottom(100);  // keyboard
  area.removeFromBottom(100);  // spectrum analyzer

  auto grid = MakeMainGrid();

  // We only need the items for layout calculation
  for (auto i = 0; i < 32; ++i) grid.items.add(juce::GridItem());

  grid.performLayout(area);

  const auto backgroundColor =
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
  const auto colorA = backgroundColor.brighter(0.05f);
  const auto colorB = backgroundColor.darker(0.05f);

  const auto outlineColor = backgroundColor.brighter(0.2f);

  constexpr auto kNumSections = 8;
  // Paint Row 1 (Top Sections)
  for (auto i = 0; i < kNumSections; ++i) {
    const auto labelBounds = grid.items[i].currentBounds;
    const auto controlBounds = grid.items[i + kNumSections].currentBounds;
    // Row 1 sections should only span Row 0 (labels) and Row 1 (controls)
    const auto fullSectionBounds =
        labelBounds.withHeight(controlBounds.getBottom() - labelBounds.getY());

    g.setColour(i % 2 == 0 ? colorA : colorB);
    g.fillRect(fullSectionBounds);

    g.setColour(outlineColor);
    g.drawRect(fullSectionBounds, 2.0f);
  }

  // Paint Row 2 (Bottom Sections)
  // Only VCF Drive Scaling at column 4 (index 20)
  {
    constexpr auto kSectionIndex = 4;
    constexpr auto labelIndex = 16 + kSectionIndex;
    constexpr auto controlIndex = 24 + kSectionIndex;
    const auto labelBounds = grid.items[labelIndex].currentBounds;
    const auto controlBounds = grid.items[controlIndex].currentBounds;
    const auto fullSectionBounds =
        labelBounds.withHeight(controlBounds.getBottom() - labelBounds.getY());

    g.setColour(kSectionIndex % 2 == 0 ? colorA : colorB);
    g.fillRect(fullSectionBounds);

    g.setColour(outlineColor);
    g.drawRect(fullSectionBounds, 2.0f);
  }
}
}  // namespace audio_plugin