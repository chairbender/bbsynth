#include "BBSynth/PluginEditor.h"
#include "BBSynth/PluginProcessor.h"

namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
  : AudioProcessorEditor{&p},
    keyboardComponent{keyboard_state_,
                      juce::MidiKeyboardComponent::horizontalKeyboard},
    processorRef(p) {
  juce::ignoreUnused(processorRef);

  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(1600, 600);
}

void AudioPluginAudioProcessorEditor::GetNextAudioBlock(
    juce::AudioBuffer<float>& buffer) {
  spectrum_analyzer_.getNextAudioBlock(buffer);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  // LFO section
  lfo_label_.setText("LFO", juce::dontSendNotification);
  addAndMakeVisible(lfo_label_);

  rate_label_.setText("Rate", juce::dontSendNotification);
  addAndMakeVisible(rate_label_);

  rate_slider_.setSliderStyle(juce::Slider::LinearVertical);
  rate_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(rate_slider_);
  rate_attachment_ = std::make_unique<
      juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "lfoRate", rate_slider_);

  delay_time_label_.setText("Delay Time", juce::dontSendNotification);
  addAndMakeVisible(delay_time_label_);

  delay_time_slider_.setSliderStyle(juce::Slider::LinearVertical);
  delay_time_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(delay_time_slider_);
  delay_time_attachment_ = std::make_unique<
      juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "lfoDelayTimeSeconds", delay_time_slider_);

  lfo_attack_label_.setText("Attack", juce::dontSendNotification);
  addAndMakeVisible(lfo_attack_label_);

  lfo_attack_slider_.setSliderStyle(juce::Slider::LinearVertical);
  lfo_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(lfo_attack_slider_);
  lfo_attack_attachment_ = std::make_unique<
      juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "lfoAttack", lfo_attack_slider_);

  lfo_wave_form_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(lfo_wave_form_label_);

  lfo_wave_form_combo_.clear(juce::dontSendNotification);
  lfo_wave_form_combo_.addItem("sine", 1);
  lfo_wave_form_combo_.addItem("sawFall", 2);
  lfo_wave_form_combo_.addItem("triangle", 3);
  lfo_wave_form_combo_.addItem("square", 4);
  lfo_wave_form_combo_.addItem("random", 5);
  addAndMakeVisible(lfo_wave_form_combo_);
  lfo_wave_form_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "lfoWaveType", lfo_wave_form_combo_);
  lfo_wave_form_label_.attachToComponent(&lfo_wave_form_combo_, false);

  // VCO Modulator sectiong
  vco_mod_label_.setText("VCO Modulator", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_label_);

  // lfo -> freq mod
  vco_mod_lfo_freq_label_.setText("LFO Freq Mod", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_lfo_freq_label_);
  vco_mod_lfo_freq_slider_.setSliderStyle(juce::Slider::LinearVertical);
  vco_mod_lfo_freq_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  addAndMakeVisible(vco_mod_lfo_freq_slider_);
  vco_mod_freq_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "vcoModLfoFreq", vco_mod_lfo_freq_slider_);

  // env1 -> freq mod
  vco_mod_env1_freq_label_.setText("ENV1 Freq Mod", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_env1_freq_label_);
  vco_mod_env1_freq_slider_.setSliderStyle(juce::Slider::LinearVertical);
  vco_mod_env1_freq_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  addAndMakeVisible(vco_mod_env1_freq_slider_);
  vco_mod_env1_freq_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "vcoModEnv1Freq", vco_mod_env1_freq_slider_);

  // VCO pickers
  vco_mod_osc1_button_.setButtonText("VCO 1");
  vco_mod_osc1_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc1_button_);
  vco_mod_osc1_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ButtonAttachment>(
      processorRef.apvts_, "vcoModOsc1", vco_mod_osc1_button_);

  vco_mod_osc2_button_.setButtonText("VCO 2");
  vco_mod_osc2_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc2_button_);
  vco_mod_osc2_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ButtonAttachment>(
      processorRef.apvts_, "vcoModOsc2", vco_mod_osc2_button_);

  pulse_width_label_.setText("Pulse Width", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_label_);
  pulse_width_slider_.setSliderStyle(juce::Slider::LinearVertical);
  pulse_width_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  addAndMakeVisible(pulse_width_slider_);
  pulse_width_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "pulseWidth", pulse_width_slider_);

  pulse_width_source_label_.setText("PW Source", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_source_label_);
  pulse_width_source_combo_.clear(juce::dontSendNotification);
  pulse_width_source_combo_.addItemList({"E2-", "E2+", "E1-", "E1+", "LFO", "MAN"}, 1);
  addAndMakeVisible(pulse_width_source_combo_);
  pulse_width_source_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "pulseWidthSource", pulse_width_source_combo_);

  // vco1 section
  vco1_label_.setText("VCO 1", juce::dontSendNotification);
  addAndMakeVisible(vco1_label_);

  // Wave type selector
  wave_type_combo_.clear(juce::dontSendNotification);
  wave_type_combo_.addItem("sine", 1);
  wave_type_combo_.addItem("sawFall", 2);
  wave_type_combo_.addItem("triangle", 3);
  wave_type_combo_.addItem("square", 4);
  wave_type_combo_.addItem("random", 5);
  addAndMakeVisible(wave_type_combo_);
  wave_type_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "waveType", wave_type_combo_);
  wave_type_label_.setText("Waveform", juce::dontSendNotification);
  wave_type_label_.attachToComponent(&wave_type_combo_, false);
  addAndMakeVisible(wave_type_label_);

  // vco2 section
  vco2_label_.setText("VCO 2", juce::dontSendNotification);
  addAndMakeVisible(vco2_label_);

  // Wave type selector
  wave2_type_combo_.clear(juce::dontSendNotification);
  wave2_type_combo_.addItem("sine", 1);
  wave2_type_combo_.addItem("sawFall", 2);
  wave2_type_combo_.addItem("triangle", 3);
  wave2_type_combo_.addItem("square", 4);
  wave2_type_combo_.addItem("random", 5);
  addAndMakeVisible(wave2_type_combo_);
  wave2_type_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "wave2Type", wave2_type_combo_);
  wave2_type_label_.setText("Waveform", juce::dontSendNotification);
  wave2_type_label_.attachToComponent(&wave2_type_combo_, false);
  addAndMakeVisible(wave2_type_label_);

  // fine tune
  fine_tune_slider_.setSliderStyle(juce::Slider::LinearVertical);
  fine_tune_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  fine_tune_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "fineTune", fine_tune_slider_);
  fine_tune_label_.setText("Fine Tune", juce::dontSendNotification);
  fine_tune_label_.attachToComponent(&fine_tune_slider_, false);
  addAndMakeVisible(fine_tune_slider_);

  // sync
  vco2_sync_button_.setButtonText("Sync");
  vco2_sync_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco2_sync_button_);
  vco2_sync_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ButtonAttachment>(
      processorRef.apvts_, "vco2Sync", vco2_sync_button_);
  vco2_sync_label_.setText("Sync", juce::dontSendNotification);
  vco2_sync_label_.attachToComponent(&vco2_sync_button_, false);
  addAndMakeVisible(vco2_sync_label_);

  //vcf section
  vcf_label_.setText("VCF", juce::dontSendNotification);
  addAndMakeVisible(vcf_label_);

  filter_cutoff_slider_.setSliderStyle(juce::Slider::LinearVertical);
  filter_cutoff_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                        20);
  addAndMakeVisible(filter_cutoff_slider_);
  filter_cutoff_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "filterCutoffFreq", filter_cutoff_slider_);

  filter_cutoff_label_.setText("Fiter Cutoff", juce::dontSendNotification);
  filter_cutoff_label_.attachToComponent(&filter_cutoff_slider_, false);
  addAndMakeVisible(filter_cutoff_label_);

  filter_resonance_slider_.setSliderStyle(juce::Slider::LinearVertical);
  filter_resonance_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                           80, 20);
  addAndMakeVisible(filter_resonance_slider_);
  filter_resonance_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "filterResonance", filter_resonance_slider_);

  filter_resonance_label_.
      setText("Fiter Resonance", juce::dontSendNotification);
  filter_resonance_label_.attachToComponent(&filter_resonance_slider_, false);
  addAndMakeVisible(filter_resonance_label_);

  filter_drive_slider_.setSliderStyle(juce::Slider::LinearVertical);
  filter_drive_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(filter_drive_slider_);
  filter_drive_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "filterDrive", filter_drive_slider_);

  filter_drive_label_.setText("Fiter Drive", juce::dontSendNotification);
  filter_drive_label_.attachToComponent(&filter_drive_slider_, false);
  addAndMakeVisible(filter_drive_label_);

  // filter slope
  filter_slope_combo_.clear(juce::dontSendNotification);
  filter_slope_combo_.addItem("-24 dB", 1);
  filter_slope_combo_.addItem("-18 dB", 2);
  filter_slope_combo_.addItem("-12 dB", 3);
  addAndMakeVisible(filter_slope_combo_);
  filter_slope_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "filterSlope", filter_slope_combo_);
  filter_slope_label_.setText("Slope", juce::dontSendNotification);
  filter_slope_label_.attachToComponent(&filter_slope_combo_, false);
  addAndMakeVisible(filter_slope_label_);

  filter_env_mod_slider_.setSliderStyle(juce::Slider::LinearVertical);
  filter_env_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  addAndMakeVisible(filter_env_mod_slider_);
  filter_env_mod_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "filterEnvMod", filter_env_mod_slider_);
  filter_env_mod_label_.setText("Env Mod", juce::dontSendNotification);
  filter_env_mod_label_.attachToComponent(&filter_env_mod_slider_, false);
  addAndMakeVisible(filter_env_mod_label_);

  filter_env_source_combo_.clear(juce::dontSendNotification);
  filter_env_source_combo_.addItem("Env 1", 1);
  filter_env_source_combo_.addItem("Env 2", 2);
  addAndMakeVisible(filter_env_source_combo_);
  filter_env_source_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "filterEnvSource", filter_env_source_combo_);
  filter_env_source_label_.setText("Env Source", juce::dontSendNotification);
  filter_env_source_label_.attachToComponent(&filter_env_source_combo_, false);
  addAndMakeVisible(filter_env_source_label_);

  // env1 section
  env1_label_.setText("ENV1", juce::dontSendNotification);
  addAndMakeVisible(env1_label_);
  env1_attack_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env1_attack_slider_.
      setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_attack_slider_);
  env1_attack_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrAttack", env1_attack_slider_);
  env1_attack_label_.setText("Attack", juce::dontSendNotification);
  env1_attack_label_.attachToComponent(&env1_attack_slider_, false);
  addAndMakeVisible(env1_attack_label_);

  env1_decay_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env1_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_decay_slider_);
  env1_decay_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrDecay", env1_decay_slider_);
  env1_decay_label_.setText("Decay", juce::dontSendNotification);
  env1_decay_label_.attachToComponent(&env1_decay_slider_, false);
  addAndMakeVisible(env1_decay_label_);

  env1_sustain_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env1_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_sustain_slider_);
  env1_sustain_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrSustain", env1_sustain_slider_);
  env1_sustain_label_.setText("Sustain", juce::dontSendNotification);
  env1_sustain_label_.attachToComponent(&env1_sustain_slider_, false);
  addAndMakeVisible(env1_sustain_label_);

  env1_release_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env1_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env1_release_slider_);
  env1_release_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrRelease", env1_release_slider_);
  env1_release_label_.setText("Release", juce::dontSendNotification);
  env1_release_label_.attachToComponent(&env1_release_slider_, false);
  addAndMakeVisible(env1_release_label_);

  // env2 section
  env2_label_.setText("ENV2", juce::dontSendNotification);
  addAndMakeVisible(env2_label_);
  env2_attack_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env2_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env2_attack_slider_);
  env2_attack_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "env2Attack", env2_attack_slider_);
  env2_attack_label_.setText("Attack", juce::dontSendNotification);
  env2_attack_label_.attachToComponent(&env2_attack_slider_, false);
  addAndMakeVisible(env2_attack_label_);

  env2_decay_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env2_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env2_decay_slider_);
  env2_decay_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "env2Decay", env2_decay_slider_);
  env2_decay_label_.setText("Decay", juce::dontSendNotification);
  env2_decay_label_.attachToComponent(&env2_decay_slider_, false);
  addAndMakeVisible(env2_decay_label_);

  env2_sustain_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env2_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_sustain_slider_);
  env2_sustain_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "env2Sustain", env2_sustain_slider_);
  env2_sustain_label_.setText("Sustain", juce::dontSendNotification);
  env2_sustain_label_.attachToComponent(&env2_sustain_slider_, false);
  addAndMakeVisible(env2_sustain_label_);

  env2_release_slider_.setSliderStyle(juce::Slider::LinearVertical);
  env2_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                       20);
  addAndMakeVisible(env2_release_slider_);
  env2_release_attachment_ = std::make_unique<
    juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "env2Release", env2_release_slider_);
  env2_release_label_.setText("Release", juce::dontSendNotification);
  env2_release_label_.attachToComponent(&env2_release_slider_, false);
  addAndMakeVisible(env2_release_label_);

  addAndMakeVisible(spectrum_analyzer_);

  addAndMakeVisible(keyboardComponent);
}

void AudioPluginAudioProcessorEditor::resized() {
  // Layout
  auto area = getLocalBounds();

  // Keep spectrum analyzer and keyboard at the bottom as-is
  keyboardComponent.setBounds(area.removeFromBottom(100));
  spectrum_analyzer_.setBounds(area.removeFromBottom(100));

  // Use a Grid to place the three sections (VCO1, VCF, ENV1) in a single row
  auto topRow = area; // remaining area after removing bottom components

  juce::Grid grid;
  // todod probably dont need both auto + template
  grid.alignContent = juce::Grid::AlignContent::center;
  grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(4))
  };
  grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
  };

  grid.items = {
      juce::GridItem(lfo_label_),
      juce::GridItem(vco_mod_label_),
      juce::GridItem(vco1_label_),
      juce::GridItem(vco2_label_),
      juce::GridItem(vcf_label_),
      juce::GridItem(env1_label_),
      juce::GridItem(env2_label_),
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{}
  };

  grid.performLayout(topRow);

  auto item_idx = grid.items.size() / 2;

  // LFO section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{rate_slider_},
      juce::GridItem{delay_time_slider_},
      juce::GridItem{lfo_attack_slider_},
      juce::GridItem{lfo_wave_form_combo_},
      juce::GridItem{rate_label_},
      juce::GridItem{delay_time_label_},
      juce::GridItem{lfo_attack_label_},
      juce::GridItem{lfo_wave_form_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // VCO mod
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{vco_mod_lfo_freq_slider_},
      juce::GridItem{vco_mod_env1_freq_slider_},
      juce::GridItem{vco_mod_osc1_button_},
      juce::GridItem{vco_mod_osc2_button_},
      juce::GridItem{pulse_width_slider_},
      juce::GridItem{pulse_width_source_combo_},
      juce::GridItem{vco_mod_lfo_freq_label_},
      juce::GridItem{vco_mod_env1_freq_label_},
      juce::GridItem{},
      juce::GridItem{},
      juce::GridItem{pulse_width_label_},
      juce::GridItem{pulse_width_source_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // VCO1 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{wave_type_combo_},
      juce::GridItem{wave_type_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // VCO2 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{wave2_type_combo_},
      juce::GridItem{fine_tune_slider_},
      juce::GridItem{vco2_sync_button_},
      juce::GridItem{wave2_type_label_},
      juce::GridItem{fine_tune_label_},
      juce::GridItem{vco2_sync_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // VCF section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{filter_cutoff_slider_},
      juce::GridItem{filter_resonance_slider_},
      juce::GridItem{filter_drive_slider_},
      juce::GridItem{filter_slope_combo_},
      juce::GridItem{filter_env_mod_slider_},
      juce::GridItem{filter_env_source_combo_},
      juce::GridItem{filter_cutoff_label_},
      juce::GridItem{filter_resonance_label_},
      juce::GridItem{filter_drive_label_},
      juce::GridItem{filter_slope_label_},
      juce::GridItem{filter_env_mod_label_},
      juce::GridItem{filter_env_source_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // ENV1 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{env1_attack_slider_},
      juce::GridItem{env1_decay_slider_},
      juce::GridItem{env1_sustain_slider_},
      juce::GridItem{env1_release_slider_},
      juce::GridItem{env1_attack_label_},
      juce::GridItem{env1_decay_label_},
      juce::GridItem{env1_sustain_label_},
      juce::GridItem{env1_release_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // ENV2 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.autoColumns = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Fr(1));
    section_grid.templateColumns = {
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.templateRows = {
      juce::Grid::TrackInfo(juce::Grid::Fr(4)),
      juce::Grid::TrackInfo(juce::Grid::Fr(1))
    };
    section_grid.items = {
      juce::GridItem{env2_attack_slider_},
      juce::GridItem{env2_decay_slider_},
      juce::GridItem{env2_sustain_slider_},
      juce::GridItem{env2_release_slider_},
      juce::GridItem{env2_attack_label_},
      juce::GridItem{env2_decay_label_},
      juce::GridItem{env2_sustain_label_},
      juce::GridItem{env2_release_label_}
    };

    section_grid.performLayout(section_bounds.toNearestInt());
  }
}
} // namespace audio_plugin