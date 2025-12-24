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
  setSize(800, 600);
}

void AudioPluginAudioProcessorEditor::GetNextAudioBlock(
    juce::AudioBuffer<float>& buffer) {
  spectrum_analyzer_.getNextAudioBlock(buffer);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

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
  wave_type_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processorRef.apvts_, "waveType", wave_type_combo_);
  wave_type_label_.setText("Waveform", juce::dontSendNotification);
  wave_type_label_.attachToComponent(&wave_type_combo_, false);
  addAndMakeVisible(wave_type_label_);

  //vcf section
  vcf_label_.setText("VCF", juce::dontSendNotification);
  addAndMakeVisible(vcf_label_);

  filter_cutoff_slider_.setSliderStyle(juce::Slider::Rotary);
  filter_cutoff_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(filter_cutoff_slider_);
  filter_cutoff_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    processorRef.apvts_, "filterCutoffFreq", filter_cutoff_slider_);

  filter_cutoff_label_.setText("Fiter Cutoff", juce::dontSendNotification);
  filter_cutoff_label_.attachToComponent(&filter_cutoff_slider_, false);
  addAndMakeVisible(filter_cutoff_label_);

  filter_resonance_slider_.setSliderStyle(juce::Slider::Rotary);
  filter_resonance_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(filter_resonance_slider_);
  filter_resonance_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    processorRef.apvts_, "filterResonance", filter_resonance_slider_);

  filter_resonance_label_.setText("Fiter Resonance", juce::dontSendNotification);
  filter_resonance_label_.attachToComponent(&filter_resonance_slider_, false);
  addAndMakeVisible(filter_resonance_label_);

  filter_drive_slider_.setSliderStyle(juce::Slider::Rotary);
  filter_drive_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(filter_drive_slider_);
  filter_drive_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    processorRef.apvts_, "filterDrive", filter_drive_slider_);

  filter_drive_label_.setText("Fiter Drive", juce::dontSendNotification);
  filter_drive_label_.attachToComponent(&filter_drive_slider_, false);
  addAndMakeVisible(filter_drive_label_);

  // env1 section
  env1_label_.setText("ENV1", juce::dontSendNotification);
  addAndMakeVisible(env1_label_);
  env1_attack_slider_.setSliderStyle(juce::Slider::Rotary);
  env1_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_attack_slider_);
  env1_attack_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrAttack", env1_attack_slider_);
  env1_attack_label_.setText("Attack", juce::dontSendNotification);
  env1_attack_label_.attachToComponent(&env1_attack_slider_, false);
  addAndMakeVisible(env1_attack_label_);

  env1_decay_slider_.setSliderStyle(juce::Slider::Rotary);
  env1_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_decay_slider_);
  env1_decay_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrDecay", env1_decay_slider_);
  env1_decay_label_.setText("Decay", juce::dontSendNotification);
  env1_decay_label_.attachToComponent(&env1_decay_slider_, false);
  addAndMakeVisible(env1_decay_label_);

  env1_sustain_slider_.setSliderStyle(juce::Slider::Rotary);
  env1_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_sustain_slider_);
  env1_sustain_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrSustain", env1_sustain_slider_);
  env1_sustain_label_.setText("Sustain", juce::dontSendNotification);
  env1_sustain_label_.attachToComponent(&env1_sustain_slider_, false);
  addAndMakeVisible(env1_sustain_label_);

  env1_release_slider_.setSliderStyle(juce::Slider::Rotary);
  env1_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(env1_release_slider_);
  env1_release_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrRelease", env1_release_slider_);
  env1_release_label_.setText("Release", juce::dontSendNotification);
  env1_release_label_.attachToComponent(&env1_release_slider_, false);
  addAndMakeVisible(env1_release_label_);

  addAndMakeVisible(spectrum_analyzer_);



  addAndMakeVisible(keyboardComponent);
}

void AudioPluginAudioProcessorEditor::resized() {
  // todo filter cutoff and freq params as juce params

  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  auto area = getLocalBounds();

  // keyboard / spectrum section
  keyboardComponent.setBounds(area.removeFromBottom(100));
  spectrum_analyzer_.setBounds(area.removeFromBottom(100));

  // top section
  auto top_row = area.removeFromTop(200);

  // vco 1 section
  auto vco_1_section = top_row.removeFromLeft(200);
  vco1_label_.setBounds(vco_1_section.removeFromTop(50));
  auto wave_type_stack = vco_1_section.removeFromLeft(50);
  wave_type_label_.setBounds(wave_type_stack.removeFromTop(50));
  wave_type_combo_.setBounds(wave_type_stack.removeFromTop(50));

  // vcf section
  auto vcf_section = top_row.removeFromLeft(200);
  vcf_label_.setBounds(vcf_section.removeFromTop(50));
  auto freq_stack = vcf_section.removeFromLeft(50);
  filter_cutoff_label_.setBounds(freq_stack.removeFromTop(50));
  filter_cutoff_slider_.setBounds(freq_stack.removeFromTop(50));
  auto res_stack = freq_stack.removeFromLeft(50);
  filter_resonance_label_.setBounds(res_stack.removeFromTop(50));
  filter_resonance_slider_.setBounds(res_stack.removeFromTop(50));
  auto drive_stack = res_stack.removeFromLeft(50);
  filter_drive_label_.setBounds(drive_stack.removeFromTop(50));
  filter_drive_slider_.setBounds(drive_stack.removeFromTop(50));

  // env1 section
  auto env1_section = top_row.removeFromLeft(200);
  env1_label_.setBounds(env1_section.removeFromTop(50));
  auto attack_stack = env1_section.removeFromLeft(50);
  env1_attack_slider_.setBounds(attack_stack.removeFromTop(50));
  env1_attack_label_.setBounds(attack_stack.removeFromTop(50));
  auto decay_stack = env1_section.removeFromLeft(50);
  env1_decay_slider_.setBounds(decay_stack.removeFromTop(50));
  env1_decay_label_.setBounds(decay_stack.removeFromTop(50));
  auto sustain_stack = env1_section.removeFromLeft(50);
  env1_sustain_slider_.setBounds(sustain_stack.removeFromTop(50));
  env1_sustain_label_.setBounds(sustain_stack.removeFromTop(50));
  auto release_stack = env1_section.removeFromLeft(50);
  env1_release_slider_.setBounds(release_stack.removeFromTop(50));
  env1_release_label_.setBounds(release_stack.removeFromTop(50));
}
}  // namespace audio_plugin
