#include "BBSynth/PluginEditor.h"
#include "BBSynth/PluginProcessor.h"

namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
    : AudioProcessorEditor{&p},
      keyboardComponent{keyboardState,
                        juce::MidiKeyboardComponent::horizontalKeyboard},
      processorRef(p) {
  juce::ignoreUnused(processorRef);

  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(400, 300);
}

void AudioPluginAudioProcessorEditor::getNextAudioBlock(
    juce::AudioBuffer<float>& buffer) {
  spectrumAnalyzer.getNextAudioBlock(buffer);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  g.drawFittedText("Hello World!", getLocalBounds(),
                   juce::Justification::centred, 1);

  centOffsetSlider.setSliderStyle(juce::Slider::Rotary);
  centOffsetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
  addAndMakeVisible(centOffsetSlider);

  centOffsetLabel.setText("Cent Offset", juce::dontSendNotification);
  centOffsetLabel.attachToComponent(&centOffsetSlider, false);
  addAndMakeVisible(centOffsetLabel);

  addAndMakeVisible(spectrumAnalyzer);

  centOffsetAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.apvts_, "centOffset", centOffsetSlider);

  addAndMakeVisible(keyboardComponent);
}

void AudioPluginAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  auto area = getLocalBounds().reduced(20);

  centOffsetSlider.setBounds(area.removeFromLeft(150).removeFromTop(100));

  keyboardComponent.setBounds(0, 150, 400, 150);

  spectrumAnalyzer.setBounds(0, 0, 400, 150);
}
}  // namespace audio_plugin
