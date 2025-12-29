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
        wave_type_buttons_[index]->setToggleState(true, juce::dontSendNotification);
      });
  wave_type_attachment_->sendInitialUpdate();

  const juce::StringArray lfoWaveTypeOptions = {"SIN", "SAW", "TRI", "SQR", "RND"};
  for (int i = 0; i < lfoWaveTypeOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(lfoWaveTypeOptions[i]);
    btn->setRadioGroupId(1002);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("lfoWaveType");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    lfo_wave_type_buttons_.emplace_back(std::move(btn));
  }
  lfo_wave_type_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("lfoWaveType"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        lfo_wave_type_buttons_[index]->setToggleState(true, juce::dontSendNotification);
      });
  lfo_wave_type_attachment_->sendInitialUpdate();

  const juce::StringArray wave2TypeOptions = {"SIN", "SAW", "TRI", "SQR", "RND"};
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
        wave2_type_buttons_[index]->setToggleState(true, juce::dontSendNotification);
      });
  wave2_type_attachment_->sendInitialUpdate();

  const juce::StringArray pwSourceOptions = {"E2-", "E2+", "E1-", "E1+", "LFO", "MAN"};
  for (int i = 0; i < pwSourceOptions.size(); ++i) {
    auto btn = std::make_unique<juce::ToggleButton>(pwSourceOptions[i]);
    btn->setRadioGroupId(1004);
    btn->setClickingTogglesState(false);
    btn->onClick = [this, i] {
      auto* param = processorRef.apvts_.getParameter("pulseWidthSource");
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
    };
    pulse_width_source_buttons_.emplace_back(std::move(btn));
  }
  pulse_width_source_attachment_ = std::make_unique<juce::ParameterAttachment>(
      *processorRef.apvts_.getParameter("pulseWidthSource"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        pulse_width_source_buttons_[index]->setToggleState(true, juce::dontSendNotification);
      });
  pulse_width_source_attachment_->sendInitialUpdate();

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
        filter_slope_buttons_[index]->setToggleState(true, juce::dontSendNotification);
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
      *processorRef.apvts_.getParameter("filterEnvSource"), [this](const float f) {
        const size_t index = static_cast<size_t>(f);
        filter_env_source_buttons_[index]->setToggleState(true, juce::dontSendNotification);
      });
  filter_env_source_attachment_->sendInitialUpdate();

  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(1600, 600);
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

void AudioPluginAudioProcessorEditor::parameterChanged(
    const juce::String& parameterID, float newValue) {
  juce::ignoreUnused(parameterID, newValue);
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

  rate_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  rate_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(rate_slider_);
  rate_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "lfoRate", rate_slider_);

  delay_time_label_.setText("Delay Time", juce::dontSendNotification);
  addAndMakeVisible(delay_time_label_);

  delay_time_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  delay_time_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(delay_time_slider_);
  delay_time_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "lfoDelayTimeSeconds", delay_time_slider_);

  lfo_attack_label_.setText("Attack", juce::dontSendNotification);
  addAndMakeVisible(lfo_attack_label_);

  lfo_attack_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  lfo_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(lfo_attack_slider_);
  lfo_attack_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "lfoAttack", lfo_attack_slider_);

  lfo_wave_form_label_.setText("Waveform", juce::dontSendNotification);
  addAndMakeVisible(lfo_wave_form_label_);

  for (auto& btn : lfo_wave_type_buttons_) {
    addAndMakeVisible(*btn);
  }

  processorRef.apvts_.addParameterListener("lfoWaveType", this);

  // VCO Modulator sectiong
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
          processorRef.apvts_, "vcoModLfoFreq", vco_mod_lfo_freq_slider_);

  // env1 -> freq mod
  vco_mod_env1_freq_label_.setText("ENV1 Freq Mod", juce::dontSendNotification);
  addAndMakeVisible(vco_mod_env1_freq_label_);
  vco_mod_env1_freq_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  vco_mod_env1_freq_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                            50, 20);
  addAndMakeVisible(vco_mod_env1_freq_slider_);
  vco_mod_env1_freq_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "vcoModEnv1Freq", vco_mod_env1_freq_slider_);

  // VCO pickers
  vco_mod_osc1_button_.setButtonText("VCO 1");
  vco_mod_osc1_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc1_button_);
  vco_mod_osc1_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processorRef.apvts_, "vcoModOsc1", vco_mod_osc1_button_);

  vco_mod_osc2_button_.setButtonText("VCO 2");
  vco_mod_osc2_button_.setClickingTogglesState(true);
  addAndMakeVisible(vco_mod_osc2_button_);
  vco_mod_osc2_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processorRef.apvts_, "vcoModOsc2", vco_mod_osc2_button_);

  pulse_width_label_.setText("Pulse Width", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_label_);
  pulse_width_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  pulse_width_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                      20);
  addAndMakeVisible(pulse_width_slider_);
  pulse_width_attachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "pulseWidth", pulse_width_slider_);

  pulse_width_source_label_.setText("PW Source", juce::dontSendNotification);
  addAndMakeVisible(pulse_width_source_label_);
  for (auto& btn : pulse_width_source_buttons_) {
    addAndMakeVisible(*btn);
  }
  processorRef.apvts_.addParameterListener("pulseWidthSource", this);

  // vco1 section
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

  // vco2 section
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

  // vcf section
  vcf_label_.setText("VCF", juce::dontSendNotification);
  addAndMakeVisible(vcf_label_);

  filter_hpf_slider_.setSliderStyle(juce::Slider::LinearBarVertical);
  filter_hpf_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                                        20);
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

  filter_resonance_label_.setText("Resonance",
                                  juce::dontSendNotification);
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

  // VCA section
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
  vca_lfo_mod_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
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

  // env1 section
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

  // env2 section
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
  auto topRow = area;  // remaining area after removing bottom components

  juce::Grid grid;
  // todod probably dont need both auto + template
  grid.alignContent = juce::Grid::AlignContent::center;
  grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                       juce::Grid::TrackInfo(juce::Grid::Fr(7))};
  grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(3)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(3)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(5)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(3)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(2))};

  grid.items = {juce::GridItem(lfo_label_),
                juce::GridItem(vco_mod_label_),
                juce::GridItem(vco1_label_),
                juce::GridItem(vco2_label_),
                juce::GridItem(vcf_label_),
                juce::GridItem(vca_label_),
                juce::GridItem(env1_label_),
                juce::GridItem(env2_label_),
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{},
                juce::GridItem{}};

  grid.performLayout(topRow);

  auto item_idx = grid.items.size() / 2;

  // LFO section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(3))};
    section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                 juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.items = {juce::GridItem{rate_slider_},
                          juce::GridItem{delay_time_slider_},
                          juce::GridItem{lfo_attack_slider_},
                          juce::GridItem{},
                          juce::GridItem{rate_label_},
                          juce::GridItem{delay_time_label_},
                          juce::GridItem{lfo_attack_label_},
                          juce::GridItem{lfo_wave_form_label_}};

    section_grid.performLayout(section_bounds.toNearestInt());

    auto radio_area = section_grid.items[3].currentBounds.toNearestInt();
    const auto button_height = radio_area.getHeight() / 10;

    for (auto& btn : lfo_wave_type_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }

  // VCO mod
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(2)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(4))};
    section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                 juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.items = {juce::GridItem{vco_mod_lfo_freq_slider_},
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

    auto button_area = section_grid.items[2].currentBounds;
    button_area.removeFromBottom(button_area.getHeight() / 2);
    vco_mod_osc1_button_.setBounds(
        button_area.removeFromTop(button_area.getHeight() / 2).toNearestInt());
    vco_mod_osc2_button_.setBounds(button_area.toNearestInt());

    auto radio_area = section_grid.items[4].currentBounds.toNearestInt();
    const auto button_height = radio_area.getHeight() / 2 / static_cast<int>(pulse_width_source_buttons_.size());

    for (auto& btn : pulse_width_source_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }
  }

  // VCO1 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
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

  // VCO2 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.alignContent = juce::Grid::AlignContent::center;
    section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                 juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.items = {
        juce::GridItem{cross_mod_slider_},
        juce::GridItem{},
        juce::GridItem{fine_tune_slider_},
        juce::GridItem{vco2_sync_button_},
        juce::GridItem{vco2_level_slider_},
        juce::GridItem{cross_mod_label_},
        juce::GridItem{wave2_type_label_},
        juce::GridItem{fine_tune_label_},
        juce::GridItem{},
        juce::GridItem{vco2_level_label_}};

    section_grid.performLayout(section_bounds.toNearestInt());

    auto radio_area = section_grid.items[1].currentBounds.toNearestInt();
    const auto button_height = radio_area.getHeight() / 10;

    for (auto& btn : wave2_type_buttons_) {
      btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
    }


  }

  // VCF section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
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
      const auto button_height = radio_area.getHeight() / static_cast<int>(filter_slope_buttons_.size());

      for (auto& btn : filter_slope_buttons_) {
        btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
      }
    }

    // filter env source layout
    {
      auto radio_area = section_grid.items[7].currentBounds.toNearestInt();
      const auto button_height = radio_area.getHeight() / static_cast<int>(filter_env_source_buttons_.size());

      for (auto& btn : filter_env_source_buttons_) {
        btn->setBounds(radio_area.removeFromTop(button_height).toNearestInt());
      }
    }
  }

  // VCA section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
    juce::Grid section_grid;
    section_grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                                    juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(4)),
                                 juce::Grid::TrackInfo(juce::Grid::Fr(1))};
    section_grid.items = {juce::GridItem{vca_level_slider_},
                          juce::GridItem{vca_lfo_mod_slider_},
                          juce::GridItem{vca_tone_slider_},
                          juce::GridItem{vca_level_label_},
                          juce::GridItem{vca_lfo_mod_label_},
                          juce::GridItem{vca_tone_label_}};

    section_grid.performLayout(section_bounds.toNearestInt());
  }

  // ENV1 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
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

  // ENV2 section
  {
    const auto section_bounds = grid.items[item_idx++].currentBounds;
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
}
}  // namespace audio_plugin