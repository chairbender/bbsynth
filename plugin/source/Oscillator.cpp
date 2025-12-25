#include "BBSynth/Oscillator.h"

namespace audio_plugin {
constexpr auto kOversample = 2;

OscillatorSound::OscillatorSound(
    [[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) {}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice(const juce::AudioBuffer<float>& lfo_buffer)
    : waveGenerator_{lfo_buffer, env1_buffer_, env2_buffer_},
      wave2Generator_{lfo_buffer, env1_buffer_, env2_buffer_} {
  waveGenerator_.PrepareToPlay(getSampleRate() * kOversample);
  wave2Generator_.PrepareToPlay(getSampleRate() * kOversample);
  // waveGenerator_.setHardsync(false);
  waveGenerator_.set_mode(WaveGenerator::ANTIALIAS);
  wave2Generator_.set_mode(WaveGenerator::ANTIALIAS);
  filter_.set_sample_rate(getSampleRate() * kOversample);
}

bool OscillatorVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<OscillatorSound*>(sound) != nullptr;
}

void OscillatorVoice::Configure(
    const juce::AudioProcessorValueTreeState& apvts) {
  filter_.Configure(apvts);

  // Configure ADSR envelope from parameters
  envelope_.setSampleRate(getSampleRate());
  const juce::ADSR::Parameters params(
      apvts.getRawParameterValue("adsrAttack")->load(),
      apvts.getRawParameterValue("adsrDecay")->load(),
      apvts.getRawParameterValue("adsrSustain")->load(),
      apvts.getRawParameterValue("adsrRelease")->load());
  envelope_.setParameters(params);

  // Configure ADSR envelope 2 from parameters
  envelope2_.setSampleRate(getSampleRate());
  const juce::ADSR::Parameters params2(
      apvts.getRawParameterValue("env2Attack")->load(),
      apvts.getRawParameterValue("env2Decay")->load(),
      apvts.getRawParameterValue("env2Sustain")->load(),
      apvts.getRawParameterValue("env2Release")->load());
  envelope2_.setParameters(params2);

  if (apvts.getRawParameterValue("vcoModOsc1")->load() > 0) {
    waveGenerator_.set_pitch_bend_lfo_mod(
        apvts.getRawParameterValue("vcoModLfoFreq")->load());
    waveGenerator_.set_pitch_bend_env1_mod(
        apvts.getRawParameterValue("vcoModEnv1Freq")->load());
  }

  if (apvts.getRawParameterValue("vcoModOsc2")->load() > 0) {
    wave2Generator_.set_pitch_bend_lfo_mod(
        apvts.getRawParameterValue("vcoModLfoFreq")->load());
    wave2Generator_.set_pitch_bend_env1_mod(
        apvts.getRawParameterValue("vcoModEnv1Freq")->load());
  }

  switch (static_cast<int>(apvts.getRawParameterValue("waveType")->load())) {
    case 0:
      waveGenerator_.set_wave_type(WaveGenerator::sine);
      break;
    case 1:
      waveGenerator_.set_wave_type(WaveGenerator::sawFall);
      break;
    case 2:
      waveGenerator_.set_wave_type(WaveGenerator::triangle);
      break;
    case 3:
      waveGenerator_.set_wave_type(WaveGenerator::square);
      break;
    case 4:
      waveGenerator_.set_wave_type(WaveGenerator::random);
      break;
    default:
      break;
  }

  switch (static_cast<int>(apvts.getRawParameterValue("wave2Type")->load())) {
    case 0:
      wave2Generator_.set_wave_type(WaveGenerator::sine);
      break;
    case 1:
      wave2Generator_.set_wave_type(WaveGenerator::sawFall);
      break;
    case 2:
      wave2Generator_.set_wave_type(WaveGenerator::triangle);
      break;
    case 3:
      wave2Generator_.set_wave_type(WaveGenerator::square);
      break;
    case 4:
      wave2Generator_.set_wave_type(WaveGenerator::random);
      break;
    default:
      break;
  }
  wave2Generator_.set_pitch_offset_hz(
      static_cast<double>(apvts.getRawParameterValue("fineTune")->load()));
  wave2Generator_.set_hardsync(
      apvts.getRawParameterValue("vco2Sync")->load() > 0.5f);

  const int pulseWidthSource =
      static_cast<int>(apvts.getRawParameterValue("pulseWidthSource")->load());
  switch (pulseWidthSource) {
    case 0:
      waveGenerator_.set_pulse_width_mod_type(env2Minus);
      wave2Generator_.set_pulse_width_mod_type(env2Minus);
      break;
    case 1:
      waveGenerator_.set_pulse_width_mod_type(env2Plus);
      wave2Generator_.set_pulse_width_mod_type(env2Plus);
      break;
    case 2:
      waveGenerator_.set_pulse_width_mod_type(env1Minus);
      wave2Generator_.set_pulse_width_mod_type(env1Minus);
      break;
    case 3:
      waveGenerator_.set_pulse_width_mod_type(env1Plus);
      wave2Generator_.set_pulse_width_mod_type(env1Plus);
      break;
    case 4:
      waveGenerator_.set_pulse_width_mod_type(lfo);
      wave2Generator_.set_pulse_width_mod_type(lfo);
      break;
    case 5:
      waveGenerator_.set_pulse_width_mod_type(manual);
      wave2Generator_.set_pulse_width_mod_type(manual);
      break;
  }
  const double pulseWidth =
      static_cast<double>(apvts.getRawParameterValue("pulseWidth")->load());
  waveGenerator_.set_pulse_width_mod(pulseWidth);
  wave2Generator_.set_pulse_width_mod(pulseWidth);
}

void OscillatorVoice::SetBlockSize(const int blockSize) {
  downsampler_.prepare(getSampleRate(), blockSize);
  const auto oversample_samples = blockSize * kOversample;
  oversample_buffer_.setSize(1, oversample_samples, false, true);
  env1_buffer_.setSize(1, blockSize, false, true);
  env2_buffer_.setSize(1, blockSize, false, true);
}

void OscillatorVoice::startNote(const int midiNoteNumber,
                                [[maybe_unused]] const float velocity,
                                [[maybe_unused]] juce::SynthesiserSound* sound,
                                [[maybe_unused]] int pitchWheelPos) {
  waveGenerator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  waveGenerator_.set_volume(0);
  wave2Generator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  wave2Generator_.set_volume(0);
  envelope_.noteOn();
  envelope2_.noteOn();
}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                               [[maybe_unused]] const bool allowTailOff) {
  envelope_.noteOff();
  envelope2_.noteOff();
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {
}

void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                      [[maybe_unused]] int newControllerValue) {
}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      const int numSamples) {
  const auto oversample_samples = oversample_buffer_.getNumSamples();

  // TODO: how does this interact with note on? Does this mean envelope always
  //  starts at start of a block even if it "should" start mid-block?
  // fill envelope buffers
  auto* env1_data_write = env1_buffer_.getWritePointer(0);
  auto* env2_data_write = env2_buffer_.getWritePointer(0);
  for (int i = 0; i < numSamples; ++i) {
    env1_data_write[i] = envelope_.getNextSample();
    env2_data_write[i] = envelope2_.getNextSample();
  }

  // note this will fill and process only the left channel since we want to work
  // in mono until the last moment the wave generator and filter are already
  // configured to generate at 2x oversampling.
  waveGenerator_.RenderNextBlock(oversample_buffer_, 0, oversample_samples);
  wave2Generator_.RenderNextBlock(oversample_buffer_, 0, oversample_samples);
  filter_.Process(oversample_buffer_, oversample_samples);

  // Apply ADSR envelope to the mono oversampled buffer (VCA)
  auto* data = oversample_buffer_.getWritePointer(0);
  for (int i = 0; i < oversample_samples; ++i) {
    data[i] *= env1_data_write[i / 2];
  }

  if (!envelope_.isActive()) {
    waveGenerator_.set_volume(-120);
    wave2Generator_.set_volume(-120);
    clearCurrentNote();
  }

  downsampler_.process(oversample_buffer_, outputBuffer, numSamples);
}
}  // namespace audio_plugin