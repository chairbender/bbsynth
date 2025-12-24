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

  cent_offset_slider_.setSliderStyle(juce::Slider::Rotary);
  cent_offset_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(cent_offset_slider_);
  cent_offset_attachment_ =
    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts_, "centOffset", cent_offset_slider_);

  cent_offset_label_.setText("Cent Offset", juce::dontSendNotification);
  cent_offset_label_.attachToComponent(&cent_offset_slider_, false);
  addAndMakeVisible(cent_offset_label_);

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

  filter_enabled_button_.setButtonText("Filter Enabled");
  filter_enabled_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
    processorRef.apvts_, "filterEnabled", filter_enabled_button_);
  addAndMakeVisible(filter_enabled_button_);

  filter_enabled_label_.setText("Filter Enabled", juce::dontSendNotification);
  filter_enabled_label_.attachToComponent(&filter_enabled_button_, false);
  addAndMakeVisible(filter_enabled_label_);

  // ADSR controls
  adsr_attack_slider_.setSliderStyle(juce::Slider::Rotary);
  adsr_attack_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(adsr_attack_slider_);
  adsr_attack_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrAttack", adsr_attack_slider_);
  adsr_attack_label_.setText("Attack", juce::dontSendNotification);
  adsr_attack_label_.attachToComponent(&adsr_attack_slider_, false);
  addAndMakeVisible(adsr_attack_label_);

  adsr_decay_slider_.setSliderStyle(juce::Slider::Rotary);
  adsr_decay_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(adsr_decay_slider_);
  adsr_decay_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrDecay", adsr_decay_slider_);
  adsr_decay_label_.setText("Decay", juce::dontSendNotification);
  adsr_decay_label_.attachToComponent(&adsr_decay_slider_, false);
  addAndMakeVisible(adsr_decay_label_);

  adsr_sustain_slider_.setSliderStyle(juce::Slider::Rotary);
  adsr_sustain_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(adsr_sustain_slider_);
  adsr_sustain_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrSustain", adsr_sustain_slider_);
  adsr_sustain_label_.setText("Sustain", juce::dontSendNotification);
  adsr_sustain_label_.attachToComponent(&adsr_sustain_slider_, false);
  addAndMakeVisible(adsr_sustain_label_);

  adsr_release_slider_.setSliderStyle(juce::Slider::Rotary);
  adsr_release_slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(adsr_release_slider_);
  adsr_release_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      processorRef.apvts_, "adsrRelease", adsr_release_slider_);
  adsr_release_label_.setText("Release", juce::dontSendNotification);
  adsr_release_label_.attachToComponent(&adsr_release_slider_, false);
  addAndMakeVisible(adsr_release_label_);

  addAndMakeVisible(spectrum_analyzer_);



  addAndMakeVisible(keyboardComponent);
}

void AudioPluginAudioProcessorEditor::resized() {
  // todo filter cutoff and freq params as juce params

  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  auto area = getLocalBounds().reduced(20);

  // todo this is not the right way to use area - these are mutating ops, not immutable
  cent_offset_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  filter_cutoff_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  filter_resonance_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  filter_enabled_button_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  filter_drive_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));

  // Place ADSR controls
  adsr_attack_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  adsr_decay_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  adsr_sustain_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));
  adsr_release_slider_.setBounds(area.removeFromLeft(100).removeFromTop(100));

  // Place wave selector
  wave_type_combo_.setBounds(area.removeFromLeft(150).removeFromTop(40));

  area = getLocalBounds().reduced(20);
  keyboardComponent.setBounds(0, area.getHeight() / 2, area.getWidth(), area.getHeight() / 2);

  spectrum_analyzer_.setBounds(0, area.getHeight() / 4, area.getWidth(), area.getHeight()/4);
}
}  // namespace audio_plugin
