#include "PluginEditor.h"

#include "../PluginProcessor.h"

namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
    : AudioProcessorEditor{&p},
      keyboardComponent{keyboard_state_,
                        juce::MidiKeyboardComponent::horizontalKeyboard},
      processorRef(p),
      lfo_section_{p},
      vco_mod_section_{p} {
  juce::ignoreUnused(processorRef);

  addAndMakeVisible(lfo_section_);
  addAndMakeVisible(vco_mod_section_);

  // Wave type selectors
  const juce::StringArray waveTypeOptions = {"SIN", "SAW", "TRI", "SQR", "RND"};
  for (int i = 0; i < waveTypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(waveTypeOptions[i]);
    btn->setRadioGroupId(1001);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("waveType");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    wave_type_buttons_.emplace_back(std::move(btn));
  }
  wave_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("waveType"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        wave_type_buttons_[index]->setToggleState(true,
                                                  juce::dontSendNotification);
      });
  wave_type_attachment_->sendInitialUpdate();

  const juce::StringArray wave2TypeOptions = {"SIN", "SAW", "TRI", "SQR",
                                              "RND"};
  for (int i = 0; i < wave2TypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(wave2TypeOptions[i]);
    btn->setRadioGroupId(1003);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("wave2Type");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    wave2_type_buttons_.emplace_back(std::move(btn));
  }
  wave2_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("wave2Type"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        wave2_type_buttons_[index]->setToggleState(true,
                                                   juce::dontSendNotification);
      });
  wave2_type_attachment_->sendInitialUpdate();



  const juce::StringArray filterSlopeOptions = {"-24", "-18", "-12"};
  for (int i = 0; i < filterSlopeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(filterSlopeOptions[i]);
    btn->setRadioGroupId(1005);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("filterSlope");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    filter_slope_buttons_.emplace_back(std::move(btn));
  }
  filter_slope_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("filterSlope"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        filter_slope_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  filter_slope_attachment_->sendInitialUpdate();

  const juce::StringArray filterEnvSourceOptions = {"E1", "E2"};
  for (int i = 0; i < filterEnvSourceOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(filterEnvSourceOptions[i]);
    btn->setRadioGroupId(1006);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("filterEnvSource");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    filter_env_source_buttons_.emplace_back(std::move(btn));
  }
  filter_env_source_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("filterEnvSource"),
      [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        filter_env_source_buttons_[index]->setToggleState(
            true, juce::dontSendNotification);
      });
  filter_env_source_attachment_->sendInitialUpdate();

  // VCF Drive Scaling section
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
            processorRef.apvts_, "filterInputDriveScale" + juce::String(i + 1),
            inSlider);

    auto& stateSlider =
        filter_state_drive_scale_sliders_[static_cast<size_t>(i)];
    stateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(stateSlider);
    filter_state_drive_scale_attachments_[static_cast<size_t>(i)] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.apvts_, "filterStateDriveScale" + juce::String(i + 1),
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

  setSize(1600, 900);
  centreWithSize(1600, 900);
}

void AudioPluginAudioProcessorEditor::GetNextAudioBlock(
    juce::AudioBuffer<float>& buffer) {
  spectrum_analyzer_.getNextAudioBlock(buffer);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
  processorRef.apvts_.removeParameterListener("waveType", this);
  processorRef.apvts_.removeParameterListener("lfoWaveType", this);
  processorRef.apvts_.removeParameterListener("wave2Type", this);
  processorRef.apvts_.removeParameterListener("pulseWidthSource", this);
  processorRef.apvts_.removeParameterListener("filterSlope", this);
  processorRef.apvts_.removeParameterListener("filterEnvSource", this);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  PaintBackground(g);

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  PaintVCO1Section();
  PaintVCO2Section();
  PaintVCFSection();
  PaintVCASection();
  PaintEnv1Section();
  PaintEnv2Section();

  addAndMakeVisible(spectrum_analyzer_);
  addAndMakeVisible(keyboardComponent);
}

void AudioPluginAudioProcessorEditor::LayoutVCFDriveScalingSection(
    juce::Grid grid) {
  const auto section_bounds = grid.items[28].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  // 5 columns: 1 for row labels, 4 for stages
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(2))};
  // 3 rows: 1 for headers, 1 for input, 1 for state
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(4))};
  section_grid.items = {
      // Row 0
      juce::GridItem{}, juce::GridItem{filter_stage_header_labels_[0]},
      juce::GridItem{filter_stage_header_labels_[1]},
      juce::GridItem{filter_stage_header_labels_[2]},
      juce::GridItem{filter_stage_header_labels_[3]},
      // Row 1
      juce::GridItem{filter_input_row_label_},
      juce::GridItem{filter_input_drive_scale_sliders_[0]},
      juce::GridItem{filter_input_drive_scale_sliders_[1]},
      juce::GridItem{filter_input_drive_scale_sliders_[2]},
      juce::GridItem{filter_input_drive_scale_sliders_[3]},
      // Row 2
      juce::GridItem{filter_state_row_label_},
      juce::GridItem{filter_state_drive_scale_sliders_[0]},
      juce::GridItem{filter_state_drive_scale_sliders_[1]},
      juce::GridItem{filter_state_drive_scale_sliders_[2]},
      juce::GridItem{filter_state_drive_scale_sliders_[3]}};

  section_grid.performLayout(section_bounds.toNearestInt());
}

juce::Grid AudioPluginAudioProcessorEditor::LayoutMainGrid() {
  // Layout
  auto area = getLocalBounds();

  // Keep spectrum analyzer and keyboard at the bottom as-is
  keyboardComponent.setBounds(area.removeFromBottom(100));
  spectrum_analyzer_.setBounds(area.removeFromBottom(100));

  // Use a Grid to place the three sections (VCO1, VCF, ENV1) in a single row
  auto topRow = area;  // remaining area after removing bottom components

  auto grid = MakeMainGrid();

  grid.items = {
      // row 1 labels
      juce::GridItem(lfo_section_), juce::GridItem(vco_mod_section_),
      juce::GridItem(vco1_label_), juce::GridItem(vco2_label_),
      juce::GridItem(vcf_label_), juce::GridItem(vca_label_),
      juce::GridItem(env1_label_), juce::GridItem(env2_label_),
      // row 1 controls
      juce::GridItem{}, juce::GridItem{}, juce::GridItem{}, juce::GridItem{},
      juce::GridItem{}, juce::GridItem{}, juce::GridItem{}, juce::GridItem{},
      // Row 2 labels
      juce::GridItem{}, juce::GridItem{}, juce::GridItem{}, juce::GridItem{},
      juce::GridItem(vcf_drive_scaling_label_), juce::GridItem{},
      juce::GridItem{}, juce::GridItem{},
      // Row 2 controls
      juce::GridItem{}, juce::GridItem{}, juce::GridItem{}, juce::GridItem{},
      juce::GridItem{}, juce::GridItem{}, juce::GridItem{}, juce::GridItem{}};

  grid.performLayout(topRow);
  // todo: fix after row 1+2 finally merged
  const auto row_1_height =
      grid.items[0].currentBounds.toNearestInt().getHeight();
  const auto row_2_height =
      grid.items[8].currentBounds.toNearestInt().getHeight();
  const auto combined_height = row_1_height + row_2_height;
  lfo_section_.setBounds(lfo_section_.getBounds().withHeight(combined_height));
  vco_mod_section_.setBounds(vco_mod_section_.getBounds().withHeight(combined_height));
  return grid;
}
void AudioPluginAudioProcessorEditor::LayoutVCFSection(const juce::Grid grid) {
  auto label_bounds = vcf_label_.getBounds().reduced(4, 0);
  vcf_label_.setBounds(
      label_bounds.removeFromLeft(label_bounds.getWidth() / 3));
  filter_type_combo_.setBounds(label_bounds);

  const auto section_bounds = grid.items[12].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{filter_hpf_slider_},
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
    auto radio_area = section_grid.items[4].currentBounds.toNearestInt();
    const auto button_height =
        radio_area.getHeight() / static_cast<int>(filter_slope_buttons_.size());

    for (auto& btn : filter_slope_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }

  // filter env source layout
  {
    auto radio_area = section_grid.items[7].currentBounds.toNearestInt();
    const auto button_height =
        radio_area.getHeight() /
        static_cast<int>(filter_env_source_buttons_.size());

    for (auto& btn : filter_env_source_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }
}
void AudioPluginAudioProcessorEditor::LayoutVCO2Section(const juce::Grid grid) {
  auto label_bounds = vco2_label_.getBounds().reduced(4, 0);
  vco2_label_.setBounds(
      label_bounds.removeFromLeft(label_bounds.getWidth() / 2));
  vco2_sync_button_.setBounds(label_bounds);

  const auto section_bounds = grid.items[11].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {
      juce::GridItem{cross_mod_slider_}, juce::GridItem{},
      juce::GridItem{fine_tune_slider_}, juce::GridItem{vco2_level_slider_},
      juce::GridItem{cross_mod_label_},  juce::GridItem{wave2_type_label_},
      juce::GridItem{fine_tune_label_},  juce::GridItem{vco2_level_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto radio_area = section_grid.items[1].currentBounds.toNearestInt();
  const auto button_height = radio_area.getHeight() / 10;

  for (auto& btn : wave2_type_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}
void AudioPluginAudioProcessorEditor::LayoutVCO1Section(const juce::Grid grid) {
  const auto section_bounds = grid.items[10].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};

  section_grid.items = {juce::GridItem{},  // placeholder for radio area
                        juce::GridItem{vco1_level_slider_},
                        juce::GridItem{wave_type_label_},
                        juce::GridItem{vco1_level_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());

  auto radio_area = section_grid.items[0].currentBounds.toNearestInt();
  const auto button_height = radio_area.getHeight() / 10;

  for (auto& btn : wave_type_buttons_) {
    btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
  }
}
void AudioPluginAudioProcessorEditor::LayoutVCASection(const juce::Grid grid) {
  const auto section_bounds = grid.items[13].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {
      juce::GridItem{vca_level_slider_},  juce::GridItem{vca_lfo_mod_slider_},
      juce::GridItem{vca_tone_slider_},   juce::GridItem{vca_level_label_},
      juce::GridItem{vca_lfo_mod_label_}, juce::GridItem{vca_tone_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());
}
void AudioPluginAudioProcessorEditor::LayoutEnv1Section(const juce::Grid grid) {
  const auto section_bounds = grid.items[14].currentBounds.reduced(4);
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
void AudioPluginAudioProcessorEditor::LayoutEnv2Section(const juce::Grid grid) {
  const auto section_bounds = grid.items[15].currentBounds.reduced(4);
  juce::Grid section_grid;
  section_grid.alignContent = juce::Grid::AlignContent::center;
  section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                  juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                               juce::Grid::TrackInfo(juce::Grid::Fr(1))};
  section_grid.items = {juce::GridItem{env2_attack_slider_},
                        juce::GridItem{env2_decay_slider_},
                        juce::GridItem{env2_sustain_slider_},
                        juce::GridItem{env2_release_slider_},
                        juce::GridItem{env2_attack_label_},
                        juce::GridItem{env2_decay_label_},
                        juce::GridItem{env2_sustain_label_},
                        juce::GridItem{env2_release_label_}};

  section_grid.performLayout(section_bounds.toNearestInt());
}
void AudioPluginAudioProcessorEditor::resized() {
  const auto grid = LayoutMainGrid();

  LayoutVCO1Section(grid);
  LayoutVCO2Section(grid);
  LayoutVCFSection(grid);
  LayoutVCASection(grid);
  LayoutEnv1Section(grid);
  LayoutEnv2Section(grid);

  // row 2 sections
  LayoutVCFDriveScalingSection(grid);
}

void AudioPluginAudioProcessorEditor::parameterChanged(
    const juce::String& parameterID, float newValue) {
  juce::ignoreUnused(parameterID, newValue);
}
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

void AudioPluginAudioProcessorEditor::PaintVCO1Section() {
  vco1_label_.setText("VCO 1", juce::dontSendNotification);
  addAndMakeVisible(vco1_label_);

  // Wave type selector
  for (size_t i = 0; i < wave_type_buttons_.size(); i++) {
    auto* btn = wave_type_buttons_[i].get();
    addAndMakeVisible(*btn);
  }

  // We still need something to listen to parameter changes to update buttons
  processorRef.apvts_.addParameterListener("waveType", this);

  wave_type_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(wave_type_label_);

  vco1_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco1_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vco1_level_slider_);
  vco1_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vco1Level", vco1_level_slider_);
  vco1_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vco1_level_label_);
}
void AudioPluginAudioProcessorEditor::PaintVCO2Section() {
  vco2_label_.setText("VCO 2", juce::dontSendNotification);
  addAndMakeVisible(vco2_label_);

  // Wave type selector
  for (auto& btn : wave2_type_buttons_) {
    addAndMakeVisible(*btn);
  }

  processorRef.apvts_.addParameterListener("wave2Type", this);

  wave2_type_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(wave2_type_label_);

  vco2_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco2_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vco2_level_slider_);
  vco2_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vco2Level", vco2_level_slider_);
  vco2_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vco2_level_label_);

  cross_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  cross_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(cross_mod_slider_);
  cross_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "crossMod", cross_mod_slider_);
  cross_mod_label_.setText("Cross Mod", juce::dontSendNotification);
  addAndMakeVisible(cross_mod_label_);

  // fine tune
  fine_tune_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  fine_tune_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  fine_tune_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "fineTune", fine_tune_slider_);
  fine_tune_label_.setText("Fine Tune", juce::dontSendNotification);
  addAndMakeVisible(fine_tune_label_);
  addAndMakeVisible(fine_tune_slider_);

  // sync
  vco2_sync_button_.setButtonText("Sync");
  vco2_sync_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco2_sync_button_);
  vco2_sync_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processorRef.apvts_, "vco2Sync", vco2_sync_button_);
}
void AudioPluginAudioProcessorEditor::PaintVCFSection() {
  vcf_label_.setText("VCF", juce::dontSendNotification);
  addAndMakeVisible(vcf_label_);

  filter_hpf_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_hpf_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(filter_hpf_slider_);
  filter_hpf_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "hpfFreq", filter_hpf_slider_);

  filter_hpf_label_.setText("HPF", juce::dontSendNotification);
  addAndMakeVisible(filter_hpf_label_);

  filter_cutoff_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_cutoff_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                        20);
  addAndMakeVisible(filter_cutoff_slider_);
  filter_cutoff_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "filterCutoffFreq", filter_cutoff_slider_);

  filter_cutoff_label_.setText("Cutoff", juce::dontSendNotification);
  addAndMakeVisible(filter_cutoff_label_);

  filter_resonance_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_resonance_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                           80, 20);
  addAndMakeVisible(filter_resonance_slider_);
  filter_resonance_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "filterResonance", filter_resonance_slider_);

  filter_resonance_label_.setText("Resonance", juce::dontSendNotification);
  addAndMakeVisible(filter_resonance_label_);

  filter_drive_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_drive_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(filter_drive_slider_);
  filter_drive_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "filterDrive", filter_drive_slider_);

  filter_drive_label_.setText("Drive", juce::dontSendNotification);
  addAndMakeVisible(filter_drive_label_);

  // filter slope
  for (auto& btn : filter_slope_buttons_) {
    addAndMakeVisible(*btn);
  }
  processorRef.apvts_.addParameterListener("filterSlope", this);
  filter_slope_label_.setText("Slope", juce::dontSendNotification);
  addAndMakeVisible(filter_slope_label_);

  filter_env_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_env_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                         20);
  addAndMakeVisible(filter_env_mod_slider_);
  filter_env_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "filterEnvMod", filter_env_mod_slider_);
  filter_env_mod_label_.setText("Env Mod", juce::dontSendNotification);
  addAndMakeVisible(filter_env_mod_label_);

  filter_lfo_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_lfo_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                         20);
  addAndMakeVisible(filter_lfo_mod_slider_);
  filter_lfo_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "filterLfoMod", filter_lfo_mod_slider_);
  filter_lfo_mod_label_.setText("LFO Mod", juce::dontSendNotification);
  addAndMakeVisible(filter_lfo_mod_label_);

  for (auto& btn : filter_env_source_buttons_) {
    addAndMakeVisible(*btn);
  }
  processorRef.apvts_.addParameterListener("filterEnvSource", this);
  filter_env_source_label_.setText("Env Source", juce::dontSendNotification);
  addAndMakeVisible(filter_env_source_label_);

  filter_type_combo_.clear(juce::dontSendNotification);
  filter_type_combo_.addItemList(
      {"Delayed Feedback", "TPT Newton-Raphson", "Disabled"}, 1);
  addAndMakeVisible(filter_type_combo_);
  filter_type_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processorRef.apvts_, "vcfFilterType", filter_type_combo_);
}
void AudioPluginAudioProcessorEditor::PaintVCASection() {
  vca_label_.setText("VCA", juce::dontSendNotification);
  addAndMakeVisible(vca_label_);

  vca_level_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_level_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vca_level_slider_);
  vca_level_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vcaLevel", vca_level_slider_);
  vca_level_label_.setText("Level", juce::dontSendNotification);
  addAndMakeVisible(vca_level_label_);

  vca_lfo_mod_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_lfo_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                      20);
  addAndMakeVisible(vca_lfo_mod_slider_);
  vca_lfo_mod_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vcaLfoMod", vca_lfo_mod_slider_);
  vca_lfo_mod_label_.setText("LFO Mod", juce::dontSendNotification);
  addAndMakeVisible(vca_lfo_mod_label_);

  vca_tone_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vca_tone_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(vca_tone_slider_);
  vca_tone_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vcaTone", vca_tone_slider_);
  vca_tone_label_.setText("Tone", juce::dontSendNotification);
  addAndMakeVisible(vca_tone_label_);
}
void AudioPluginAudioProcessorEditor::PaintEnv1Section() {
  env1_label_.setText("ENV1", juce::dontSendNotification);
  addAndMakeVisible(env1_label_);
  env1_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                      20);
  addAndMakeVisible(env1_attack_slider_);
  env1_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "adsrAttack", env1_attack_slider_);
  env1_attack_label_.setText("A", juce::dontSendNotification);
  addAndMakeVisible(env1_attack_label_);

  env1_decay_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_decay_slider_);
  env1_decay_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "adsrDecay", env1_decay_slider_);
  env1_decay_label_.setText("D", juce::dontSendNotification);
  addAndMakeVisible(env1_decay_label_);

  env1_sustain_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_sustain_slider_);
  env1_sustain_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "adsrSustain", env1_sustain_slider_);
  env1_sustain_label_.setText("S", juce::dontSendNotification);
  addAndMakeVisible(env1_sustain_label_);

  env1_release_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env1_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_release_slider_);
  env1_release_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "adsrRelease", env1_release_slider_);
  env1_release_label_.setText("R", juce::dontSendNotification);
  addAndMakeVisible(env1_release_label_);
}
void AudioPluginAudioProcessorEditor::PaintEnv2Section() {
  env2_label_.setText("ENV2", juce::dontSendNotification);
  addAndMakeVisible(env2_label_);
  env2_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                      20);
  addAndMakeVisible(env2_attack_slider_);
  env2_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "env2Attack", env2_attack_slider_);
  env2_attack_label_.setText("A", juce::dontSendNotification);
  addAndMakeVisible(env2_attack_label_);

  env2_decay_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env2_decay_slider_);
  env2_decay_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "env2Decay", env2_decay_slider_);
  env2_decay_label_.setText("D", juce::dontSendNotification);
  addAndMakeVisible(env2_decay_label_);

  env2_sustain_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_sustain_slider_);
  env2_sustain_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "env2Sustain", env2_sustain_slider_);
  env2_sustain_label_.setText("S", juce::dontSendNotification);
  addAndMakeVisible(env2_sustain_label_);

  env2_release_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  env2_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_release_slider_);
  env2_release_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "env2Release", env2_release_slider_);
  env2_release_label_.setText("R", juce::dontSendNotification);
  addAndMakeVisible(env2_release_label_);
}
}  // namespace audio_plugin