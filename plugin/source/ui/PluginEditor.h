#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "../PluginProcessor.h"
#include "LFOSection.h"
#include "SpectrumAnalyzerComponent.h"

namespace audio_plugin {

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::AudioProcessorValueTreeState::Listener {
public:
  explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
  /**
   * must be called by root component to update audio-reactive components
   */
  void GetNextAudioBlock(juce::AudioBuffer<float>& buffer);
  ~AudioPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void LayoutVCFDriveScalingSection(juce::Grid grid);
  juce::Grid LayoutMainGrid();
  void LayoutVCFSection(juce::Grid grid);
  void LayoutVCO2Section(juce::Grid grid);
  void LayoutVCOModSection(juce::Grid grid);
  void LayoutVCO1Section(juce::Grid grid);
  void LayoutVCASection(juce::Grid grid);
  void LayoutEnv1Section(juce::Grid grid);
  void LayoutEnv2Section(juce::Grid grid);
  void resized() override;
  void parameterChanged(const juce::String& parameterID, float newValue) override;
  // todo refactor so this can be private
  juce::MidiKeyboardState keyboard_state_;


private:
  static juce::Grid MakeMainGrid();
 void PaintBackground(juce::Graphics& g) const;
 void PaintVCOModSection();
  void PaintVCO1Section();
 void PaintVCO2Section();
  void PaintVCFSection();
 void PaintVCASection();
  void PaintEnv1Section();
 void PaintEnv2Section();

 juce::MidiKeyboardComponent keyboardComponent;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  LFOSection lfo_section_;

  // VCO Modulator section
  juce::Label vco_mod_label_;
  // lfo -> freq mod
  juce::Slider vco_mod_lfo_freq_slider_;
  juce::Label vco_mod_lfo_freq_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vco_mod_freq_attachment_;
  // env1 -> freq mod
  juce::Slider vco_mod_env1_freq_slider_;
  juce::Label vco_mod_env1_freq_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vco_mod_env1_freq_attachment_;
  // VCO picker - osc1, osc2, or both
  juce::ToggleButton vco_mod_osc1_button_;
  juce::ToggleButton vco_mod_osc2_button_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> vco_mod_osc1_attachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> vco_mod_osc2_attachment_;

  // pulse width
  juce::Slider pulse_width_slider_;
  juce::Label pulse_width_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulse_width_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> pulse_width_source_buttons_;
  juce::Label pulse_width_source_label_;
  std::unique_ptr<juce::ParameterAttachment> pulse_width_source_attachment_;

  //VCO 1 section
  juce::Label vco1_label_;

  // Wave selector
  std::vector<std::unique_ptr<juce::ToggleButton>> wave_type_buttons_;
  std::unique_ptr<juce::ParameterAttachment> wave_type_attachment_;
  juce::Label wave_type_label_;

  juce::Slider vco1_level_slider_;
  juce::Label vco1_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vco1_level_attachment_;

  //VCO 2 section
  juce::Label vco2_label_;

  // Wave selector
  std::vector<std::unique_ptr<juce::ToggleButton>> wave2_type_buttons_;
  juce::Label wave2_type_label_;
  std::unique_ptr<juce::ParameterAttachment> wave2_type_attachment_;

  juce::Slider vco2_level_slider_;
  juce::Label vco2_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vco2_level_attachment_;

  juce::Slider cross_mod_slider_;
  juce::Label cross_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cross_mod_attachment_;

  //fine tune
  juce::Slider fine_tune_slider_;
  juce::Label fine_tune_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fine_tune_attachment_;

  juce::ToggleButton vco2_sync_button_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> vco2_sync_attachment_;

  // VCF section
  juce::Label vcf_label_;
  juce::Slider filter_hpf_slider_;
  juce::Label filter_hpf_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_hpf_attachment_;
  juce::Slider filter_cutoff_slider_;
  juce::Label filter_cutoff_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_cutoff_attachment_;

  juce::Slider filter_resonance_slider_;
  juce::Label filter_resonance_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_resonance_attachment_;

  juce::Slider filter_drive_slider_;
  juce::Label filter_drive_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_drive_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> filter_slope_buttons_;
  juce::Label filter_slope_label_;
  std::unique_ptr<juce::ParameterAttachment> filter_slope_attachment_;

  juce::Slider filter_env_mod_slider_;
  juce::Label filter_env_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_env_mod_attachment_;

  juce::Slider filter_lfo_mod_slider_;
  juce::Label filter_lfo_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filter_lfo_mod_attachment_;

  std::vector<std::unique_ptr<juce::ToggleButton>> filter_env_source_buttons_;
  juce::Label filter_env_source_label_;
  std::unique_ptr<juce::ParameterAttachment> filter_env_source_attachment_;

  juce::ComboBox filter_type_combo_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filter_type_attachment_;

  // VCF Drive Scaling section
  juce::Label vcf_drive_scaling_label_;
  std::array<juce::Slider, 4> filter_input_drive_scale_sliders_;
  std::array<juce::Slider, 4> filter_state_drive_scale_sliders_;
  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> filter_input_drive_scale_attachments_;
  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> filter_state_drive_scale_attachments_;
  
  // Headers for the drive scaling table
  std::array<juce::Label, 4> filter_stage_header_labels_;
  juce::Label filter_input_row_label_;
  juce::Label filter_state_row_label_;

  // VCA section
  juce::Label vca_label_;
  juce::Slider vca_level_slider_;
  juce::Label vca_level_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vca_level_attachment_;
  juce::Slider vca_lfo_mod_slider_;
  juce::Label vca_lfo_mod_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vca_lfo_mod_attachment_;
  juce::Slider vca_tone_slider_;
  juce::Label vca_tone_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vca_tone_attachment_;

  // ENV1 section
  juce::Label env1_label_;
  juce::Slider env1_attack_slider_;
  juce::Label env1_attack_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_attack_attachment_;

  juce::Slider env1_decay_slider_;
  juce::Label env1_decay_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_decay_attachment_;

  juce::Slider env1_sustain_slider_;
  juce::Label env1_sustain_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_sustain_attachment_;

  juce::Slider env1_release_slider_;
  juce::Label env1_release_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1_release_attachment_;

  // ENV2 section
  juce::Label env2_label_;
  juce::Slider env2_attack_slider_;
  juce::Label env2_attack_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env2_attack_attachment_;

  juce::Slider env2_decay_slider_;
  juce::Label env2_decay_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env2_decay_attachment_;

  juce::Slider env2_sustain_slider_;
  juce::Label env2_sustain_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env2_sustain_attachment_;

  juce::Slider env2_release_slider_;
  juce::Label env2_release_label_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env2_release_attachment_;

  SpectrumAnalyzerComponent spectrum_analyzer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
}  // namespace audio_plugin
