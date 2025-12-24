#include "BBSynth/PluginProcessor.h"
#include "BBSynth/PluginEditor.h"
#include "BBSynth/Oscillator.h"
#include "BBSynth/WaveGenerator.h"

namespace audio_plugin {
OTAFilter::OTAFilter() : s1{0}, s2{0}, s3{0}, s4{0},
                         // todo what's really the proper way to do this?
                         tanh_in_{TanhADAA{}, TanhADAA{}, TanhADAA{},
                                  TanhADAA{}},
                         tanh_state_{TanhADAA{}, TanhADAA{}, TanhADAA{},
                                     TanhADAA{}} {
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
  : AudioProcessor(
        BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
        ),
    apvts_(*this, nullptr, "ParameterTree", CreateParameterLayout()),
    // todo is this even needed or does it defaulth/
    filter_{OTAFilter{}, OTAFilter{}} {
  for (auto i = 0; i < 1; ++i) {
    synth.addVoice(new OscillatorVoice());
  }
  synth.addSound(new OscillatorSound(apvts_));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {
}

const juce::String AudioPluginAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
  // programs, so this should be at least 1, even if you're not
  // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() {
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(
    double sampleRate,
    [[maybe_unused]] int samplesPerBlock) {
  synth.setCurrentPlaybackSampleRate(sampleRate);
}

void AudioPluginAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
  juce::ignoreUnused(index);
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // This is the place where you'd normally do the guts of your plugin's
  // audio processing...
  // Make sure to reset the state if your inner loop is processing
  // the samples and the outer loop is handling the channels.
  // Alternatively, you can process the samples with the channels
  // interleaved by keeping the same state.

  if (auto editor =
      dynamic_cast<AudioPluginAudioProcessorEditor*>(getActiveEditor())) {
    //juce::MidiBuffer incomingMidi;
    // todo do we need a separate buffer or can we append to existing?
    editor->keyboard_state_.processNextMidiBuffer(
        midiMessages, 0,
        buffer.getNumSamples(), true);

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // 4 pass OTA filter, analog emulation

    if (apvts_.getRawParameterValue("filterEnabled")->load() > 0) {
      // todo refactor to separate class?
      // todo vectorize
      const auto freq = apvts_.getRawParameterValue("filterCutoffFreq")->load();
      constexpr auto res_scale = 2.0f;
      const auto resonance = res_scale * apvts_.getRawParameterValue(
                                                   "filterResonance")
                                               ->load();
      const auto g = tanf(
          juce::MathConstants<float>::pi * freq / static_cast<float>(synth.
            getSampleRate()));
      const auto drive = apvts_.getRawParameterValue("filterDrive")->load();
      const auto scale = 1.f / drive;
      // leaky integrator for numerical stability
      const auto leak = 0.99995f;

      for (auto j = 0; j < buffer.getNumChannels(); ++j) {
        auto* input = buffer.getWritePointer(j);
        auto& filter = filter_[static_cast<size_t>(j)];

        for (auto i = 0; i < buffer.getNumSamples(); ++i) {
          const auto sample = input[i];

          // resonance feedback from output
          const auto feedback = resonance * filter.s4;

          // input with feedback compensation
          const auto u = sample - feedback;

          // stage 1
          const auto v1 = filter.tanh_in_[0].process(u * scale) * drive;
          filter.s1 = leak * filter.s1 + g * (v1 - filter.tanh_state_[0].process(
                                filter.s1 * scale) * drive);

          // stage 2
          const auto v2 = filter.tanh_in_[1].process(filter.s1 * scale) * drive;
          filter.s2 = leak * filter.s2 + g * (v2 - filter.tanh_state_[1].process(
                                filter.s2 * scale) * drive);

          // stage 3
          const auto v3 = filter.tanh_in_[2].process(filter.s2 * scale) * drive;
          filter.s3 = leak * filter.s3 + g * (v3 - filter.tanh_state_[2].process(
                                filter.s3 * scale) * drive);

          // stage 4
          const auto v4 = filter.tanh_in_[3].process(filter.s3 * scale) * drive;
          filter.s4 = leak * filter.s4 + g * (v4 - filter.tanh_state_[3].process(
                                filter.s4 * scale) * drive);

          // DC block the output - this is NOT optional
          const float out = filter.s4 - filter.dc_out_x1_ + 0.99f * filter.dc_out_y1_;
          filter.dc_out_x1_ = filter.s4;
          filter.dc_out_y1_ = out;

          input[i] = out;
        }
      }
    }

    // Or more accurately, calculate mean:
    float sum = 0;
    auto* data = buffer.getReadPointer(0);  // Check channel 0
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      sum += data[i];
    }
    float dcOffset = sum / static_cast<float>(buffer.getNumSamples());
    DBG("DC Offset: " << dcOffset);


    editor->GetNextAudioBlock(buffer);
  }

  // for (int channel = 0; channel < totalNumInputChannels; ++channel) {
  //   // todo maybe use this approach instead: auto* channelData =
  //   buffer.getWritePointer(channel);
  //
  // }
  midiMessages.clear();
}

bool AudioPluginAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
  return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
  juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data,
                                                    int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
  juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::CreateParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameterList;

  juce::NormalisableRange<float> centOffsetRange{-200.f, 200.f, 1.f};

  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "centOffset", "Cent Offset", centOffsetRange, 0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterCutoffFreq", "Filter Cutoff Frequency",
      juce::NormalisableRange(20.f, 8000.f, 1.f), 1000.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterResonance", "Filter Resonance",
      juce::NormalisableRange(0.f, 4.f, 0.01f),
      1.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterDrive", "Filter Drive", juce::NormalisableRange(0.f, 1.f, 0.01f),
      0.5f));
  parameterList.push_back(std::make_unique<juce::AudioParameterBool>(
      "filterEnabled", "Filter Enabled", true, ""));

  return {parameterList.begin(), parameterList.end()};
}

void AudioPluginAudioProcessor::parameterChanged(
    const juce::String& parameterID,
    float newValue) {
  // todo if needed
  juce::ignoreUnused(parameterID, newValue);
}
} // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}