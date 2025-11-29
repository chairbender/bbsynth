#include "BBSynth/PluginProcessor.h"
#include "BBSynth/PluginEditor.h"

#include <sys/socket.h>

namespace audio_plugin {

struct SineWaveSound : juce::SynthesiserSound {
  SineWaveSound() {}

  bool appliesToNote([[maybe_unused]] int midiNoteNumber) override { return true; }
  bool appliesToChannel([[maybe_unused]] int midiChannelNumber) override { return true; }
};

struct SineWaveVoice : juce::SynthesiserVoice {
  SineWaveVoice() {}

  bool canPlaySound(juce::SynthesiserSound* sound) override {
    return dynamic_cast<SineWaveSound*>(sound) != nullptr;
  }

  void startNote(const int midiNoteNumber,
                 const float velocity,
                 [[maybe_unused]] juce::SynthesiserSound* sound,
                 [[maybe_unused]] int pitchWheelPos) override {
    currentAngle = 0.0;
    level = static_cast<double>(velocity) * 0.15;
    tailOff = 0.0;

    auto cyclesPerSecond =
        juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    auto cyclesPerSample = cyclesPerSecond / getSampleRate();

    angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
  }

  void stopNote([[maybe_unused]] float velocity, const bool allowTailOff) override {
    if (allowTailOff) {
      if (tailOff == 0.0) {
        tailOff = 1.0;
      } else {
        clearCurrentNote();
        angleDelta = 0.0;
      }
    }
  }

  void pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) override {}
  void controllerMoved([[maybe_unused]] int controllerNumber, [[maybe_unused]] int newControllerValue) override {}

  void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, const int startSample, const int numSamples) override {
    const auto endSampleIdx{startSample + numSamples};
    auto curSampleIdx = startSample;
    if (angleDelta != 0.0) {
      if (tailOff > 0.0) {
        while (curSampleIdx < endSampleIdx) {
          const float currentSample{static_cast<float>(std::sin(currentAngle) * level * tailOff)};
          for (auto i = outputBuffer.getNumChannels(); --i >= 0;) {
            outputBuffer.addSample(i, curSampleIdx, currentSample);
          }
          currentAngle += angleDelta;
          ++curSampleIdx;
          tailOff *= 0.99;
          if (tailOff <= 0.005) {
            clearCurrentNote();
            angleDelta = 0.0;
            break;
          }
        }
      } else {
        while (curSampleIdx < endSampleIdx) {
          const auto currentSample{static_cast<float>(std::sin(currentAngle) * level)};
          for (auto i = outputBuffer.getNumChannels(); --i >= 0;) {
            outputBuffer.addSample(i, curSampleIdx, currentSample);
            currentAngle += angleDelta;
            ++curSampleIdx;
          }
        }
      }
    }
  }

private:
  double currentAngle{0.0}, angleDelta{0.0}, level{0.0}, tailOff{0.0};
};

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ) {
  for (auto i = 0; i < 4; ++i) {
    synth.addVoice(new SineWaveVoice());
  }
  synth.addSound(new SineWaveSound());
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

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
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
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

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
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
  synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
  // for (int channel = 0; channel < totalNumInputChannels; ++channel) {
  //   // todo maybe use this approach instead: auto* channelData = buffer.getWritePointer(channel);
  //
  // }
  midiMessages.clear();
}

bool AudioPluginAudioProcessor::hasEditor() const {
  return true;  // (change this to false if you choose to not supply an editor)
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
}  // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}
