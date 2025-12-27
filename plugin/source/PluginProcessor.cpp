#include "BBSynth/PluginProcessor.h"

#include "BBSynth/Oscillator.h"
#include "BBSynth/PluginEditor.h"
#include "BBSynth/WaveGenerator.h"

namespace audio_plugin {
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
      // TODO: we should have the option to not pass these buffers -
      //   they're not needed for LFO
      lfo_generator_{lfo_buffer_, lfo_buffer_, lfo_buffer_} {
  for (auto i = 0; i < 1; ++i) {
    synth.addVoice(new OscillatorVoice(lfo_buffer_));
  }
  synth.addSound(new OscillatorSound(apvts_));
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

double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
  // programs, so this should be at least 1, even if you're not
  // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }

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

void AudioPluginAudioProcessor::ConfigureLFO() {
  lfo_delay_time_s_ = static_cast<float>(
      apvts_.getRawParameterValue("lfoDelayTimeSeconds")->load());
  const float lfo_attack =
      static_cast<float>(apvts_.getRawParameterValue("lfoAttack")->load());
  lfo_ramp_step_ = 1.f / (static_cast<float>(getSampleRate()) * lfo_attack);

  lfo_rate_ =
      static_cast<float>(apvts_.getRawParameterValue("lfoRate")->load());
  lfo_generator_.set_pitch_hz(static_cast<double>(lfo_rate_));

  switch (
      static_cast<int>(apvts_.getRawParameterValue("lfoWaveType")->load())) {
    case 0:
      lfo_generator_.set_wave_type(WaveGenerator::sine);
      break;
    case 1:
      lfo_generator_.set_wave_type(WaveGenerator::sawFall);
      break;
    case 2:
      lfo_generator_.set_wave_type(WaveGenerator::triangle);
      break;
    case 3:
      lfo_generator_.set_wave_type(WaveGenerator::square);
      break;
    case 4:
      lfo_generator_.set_wave_type(WaveGenerator::random);
      break;
    default:
      break;
  }
}

void AudioPluginAudioProcessor::prepareToPlay(const double sampleRate,
                                              const int samplesPerBlock) {
  synth.setCurrentPlaybackSampleRate(sampleRate);
  lfo_buffer_.setSize(1, samplesPerBlock, false, true);
  lfo_generator_.set_mode(WaveGenerator::NO_ANTIALIAS);
  lfo_generator_.set_dc_blocker_enabled(false);
  lfo_generator_.set_volume(0);
  lfo_samples_until_start_ = -1;
  lfo_ramp_ = -1;
  lfo_generator_.PrepareToPlay(sampleRate);
  ConfigureLFO();
  // Update all voices with current parameters
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto* voice = dynamic_cast<OscillatorVoice*>(synth.getVoice(i))) {
      voice->Configure(apvts_);
      voice->SetBlockSize(samplesPerBlock);
    }
  }
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
  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();

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

  if (const auto editor =
          dynamic_cast<AudioPluginAudioProcessorEditor*>(getActiveEditor())) {
    editor->keyboard_state_.processNextMidiBuffer(midiMessages, 0,
                                                  buffer.getNumSamples(), true);

    // Update all voices with current parameters
    for (int i = 0; i < synth.getNumVoices(); ++i) {
      if (auto* voice = dynamic_cast<OscillatorVoice*>(synth.getVoice(i))) {
        voice->Configure(apvts_);
      }
    }
    // lfo params
    ConfigureLFO();

    // todo instead of clearing each block, just overwrite into it instead of adding to it
    // TODO: do we actually need to do this?
    lfo_buffer_.clear(0, 0, lfo_buffer_.getNumSamples());

    // TODO: refactor the LFO logic so it doesn't clutter up this. Use state var
    // to track the LFO state. should LFO countdown start?
    int start_lfo_sample = -1;
    if (lfo_samples_until_start_ < 0) {
      for (const auto metadata : midiMessages) {
        if (const auto message = metadata.getMessage(); message.isNoteOn()) {
          const int start_countdown_sample = metadata.samplePosition;
          // start countdown
          lfo_samples_until_start_ = static_cast<int>(
              lfo_delay_time_s_ * static_cast<float>(getSampleRate()));
          start_lfo_sample = start_countdown_sample + lfo_samples_until_start_;
          if (start_lfo_sample < buffer.getNumSamples()) {
            // start this buffer
            // todo: probably wasteful to render the lfo at such high resolution
            //  / audio rate...
            lfo_generator_.MoveAngleForwardTo(0);
            lfo_generator_.RenderNextBlock(
                lfo_buffer_, start_lfo_sample,
                buffer.getNumSamples() - start_lfo_sample);
            lfo_samples_until_start_ = 0;
          }
          break;
        }
      }
    }

    // count down the LFO
    if (lfo_samples_until_start_ > 0) {
      lfo_samples_until_start_ -= buffer.getNumSamples();
      start_lfo_sample = buffer.getNumSamples() + lfo_samples_until_start_;
      if (start_lfo_sample < buffer.getNumSamples()) {
        // start this buffer
        lfo_generator_.MoveAngleForwardTo(0);
        lfo_generator_.RenderNextBlock(
            lfo_buffer_, start_lfo_sample,
            buffer.getNumSamples() - start_lfo_sample);
        lfo_samples_until_start_ = 0;
      }
    }

    // if we started this block, start ramping where we started
    if (lfo_ramp_ < 0 && start_lfo_sample >= 0) {
      lfo_ramp_ = 0;
      auto* lfo_buffer_data = lfo_buffer_.getWritePointer(0);
      for (auto i = start_lfo_sample; i < lfo_buffer_.getNumSamples(); ++i) {
        lfo_buffer_data[i] *= lfo_ramp_;
        lfo_ramp_ += lfo_ramp_step_;
        if (lfo_ramp_ >= 1.f) {
          lfo_ramp_ = 1.f;
          break;
        }
      }
    }

    // lfo already playing this block? render it and ramp if needed
    // todo: if the LFO is supposed to end this block (due to all voices
    // stopping), technically it will keep oscillating
    //   but it will have no effect since all voices stopped, so this is fine.
    if (lfo_samples_until_start_ == 0 && start_lfo_sample < 0) {
      lfo_generator_.RenderNextBlock(lfo_buffer_, 0, buffer.getNumSamples());
      if (lfo_ramp_ < 1.f) {
        // if we are currently LFO ramping, continue it
        auto* lfo_buffer_data = lfo_buffer_.getWritePointer(0);
        for (auto i = 0; i < lfo_buffer_.getNumSamples(); ++i) {
          lfo_buffer_data[i] *= lfo_ramp_;
          lfo_ramp_ += lfo_ramp_step_;
          if (lfo_ramp_ >= 1.f) {
            lfo_ramp_ = 1.f;
            break;
          }
        }
      }
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // apply VCA
    const auto vca_level = apvts_.getRawParameterValue("vcaLevel")->load();
    const auto vca_lfo_mod = apvts_.getRawParameterValue("vcaLfoMod")->load();
    auto* buf_write = buffer.getWritePointer(0);
    auto* lfo_buf_read = lfo_buffer_.getReadPointer(0);
    for (auto i = 0; i < buffer.getNumSamples(); ++i) {
      buf_write[i] *= (vca_level + lfo_buf_read[i] * vca_lfo_mod);
    }

    // stop the LFO if no more voices
    if (lfo_samples_until_start_ == 0 && start_lfo_sample < 0) {
      bool all_voices_stopped = true;
      for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (synth.getVoice(i)->isVoiceActive()) {
          all_voices_stopped = false;
          break;
        }
      }
      if (all_voices_stopped) {
        lfo_ramp_ = -1;
        lfo_samples_until_start_ = -1;
      }
    }

    editor->GetNextAudioBlock(buffer);
  }

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

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::CreateParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameterList;

  // LFO
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "lfoRate", "LFO Rate", juce::NormalisableRange(0.01f, 100.f, .01f, .1f),
      0.2f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "lfoDelayTimeSeconds", "LFO Delay Time",
      juce::NormalisableRange(0.00f, 3.f, .01f), 0.3f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "lfoAttack", "LFO Attack",
      juce::NormalisableRange(0.001f, 1.0f, .001f), 0.01f));
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "lfoWaveType", "LFO Wave Type",
      juce::StringArray{"sine", "sawFall", "triangle", "square", "random"}, 0));

  // VCO Mod
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "vcoModLfoFreq", "LFO Freq Mod", juce::NormalisableRange(-1.f, 1.f, .01f),
      0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
    "vcoModEnv1Freq", "Env 1 Freq Mod", juce::NormalisableRange(-1.f, 1.f, .01f),
    0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterBool>(
      "vcoModOsc1", "Freq Mod Osc 1", true));
  parameterList.push_back(std::make_unique<juce::AudioParameterBool>(
      "vcoModOsc2", "Freq Mod Osc 2", true));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "pulseWidth", "Pulse Width", juce::NormalisableRange(0.f, 1.f, .01f),
      0.5f));
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "pulseWidthSource", "Pulse Width Source",
      juce::StringArray{"E2-", "E2+", "E1-", "E1+", "LFO", "MAN"}, 5));

  // vco1
  // Oscillator wave type selector
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "waveType", "Wave Type",
      juce::StringArray{"SIN", "SAW", "TRI", "SQR", "RND"}, 1));

  // vco2
  // wave type
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "wave2Type", "Wave 2 Type",
      juce::StringArray{"sine", "sawFall", "triangle", "square", "random"}, 1));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "fineTune", "Fine Tune", juce::NormalisableRange(-10.f, 10.f, 0.01f), 0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterBool>(
      "vco2Sync", "Sync (VCO1->VCO2)", false));

  // ADSR envelope parameters
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "adsrAttack", "ADSR Attack (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "adsrDecay", "ADSR Decay (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "adsrSustain", "ADSR Sustain",
      juce::NormalisableRange(0.0f, 1.0f, 0.001f), 0.7f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "adsrRelease", "ADSR Release (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

  // ENV2 parameters
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "env2Attack", "ENV2 Attack (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "env2Decay", "ENV2 Decay (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "env2Sustain", "ENV2 Sustain",
      juce::NormalisableRange(0.0f, 1.0f, 0.001f), 0.7f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "env2Release", "ENV2 Release (s)",
      juce::NormalisableRange(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterCutoffFreq", "Filter Cutoff Frequency",
      juce::NormalisableRange(20.f, 8000.f, 1.f), 4000.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterResonance", "Filter Resonance",
      juce::NormalisableRange(0.f, 4.f, 0.01f), 1.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterDrive", "Filter Drive", juce::NormalisableRange(0.f, 100.f, 0.01f, .1f),
      0.5f));
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "filterSlope", "Filter Slope", juce::StringArray{"-24 dB", "-18 dB", "-12 dB"}, 0));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterEnvMod", "Filter Env Mod", juce::NormalisableRange(-1.f, 1.f, 0.01f), 0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "filterLfoMod", "Filter LFO Mod", juce::NormalisableRange(-1.f, 1.f, 0.01f), 0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterChoice>(
      "filterEnvSource", "Filter Env Source", juce::StringArray{"Env 1", "Env 2"}, 0));

  // VCA
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "vcaLevel", "VCA Level", juce::NormalisableRange(0.f, 1.f, 0.01f), 0.8f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "vcaLfoMod", "VCA LFO Mod", juce::NormalisableRange(-1.f, 1.f, 0.01f), 0.f));
  parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
      "vcaTone", "VCA Tone", juce::NormalisableRange(-1.f, 1.f, 0.01f), 0.f));

  return {parameterList.begin(), parameterList.end()};
}

void AudioPluginAudioProcessor::parameterChanged(
    const juce::String& parameterID, float newValue) {
  // todo if needed
  juce::ignoreUnused(parameterID, newValue);
}
}  // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}