#include "BBSynth/Oscillator.h"

#include "BBSynth/Constants.h"
#include "BBSynth/Utils.h"

namespace audio_plugin {

OscillatorSound::OscillatorSound(
    [[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) {}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice(const juce::AudioBuffer<float>& lfo_buffer)
    : waveGenerator_{lfo_buffer, env1_buffer_, env2_buffer_, wave2_buffer_},
      wave2Generator_{lfo_buffer, env1_buffer_, env2_buffer_, wave2_buffer_} {
  waveGenerator_.PrepareToPlay(getSampleRate() * kOversample);
  wave2Generator_.PrepareToPlay(getSampleRate() * kOversample);
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
  envelope_.Prepare(getSampleRate());
  envelope_.Configure(apvts.getRawParameterValue("adsrAttack")->load(),
                      apvts.getRawParameterValue("adsrDecay")->load(),
                      apvts.getRawParameterValue("adsrSustain")->load(),
                      apvts.getRawParameterValue("adsrRelease")->load());
  envelope2_.Prepare(getSampleRate());
  envelope2_.Configure(apvts.getRawParameterValue("env2Attack")->load(),
                       apvts.getRawParameterValue("env2Decay")->load(),
                       apvts.getRawParameterValue("env2Sustain")->load(),
                       apvts.getRawParameterValue("env2Release")->load());

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
  const auto hard_sync = apvts.getRawParameterValue("vco2Sync")->load() > 0.5f;
  const float fine_tune = apvts.getRawParameterValue("fineTune")->load();
  if (hard_sync) {
    wave2Generator_.set_hardsync(true);
    waveGenerator_.set_hardsync(false);
  } else {
    wave2Generator_.set_hardsync(false);
    waveGenerator_.set_hardsync(false);
  }
  // todo: fine tune not working correctly when hardsync off
  wave2Generator_.set_pitch_offset_hz(static_cast<double>(fine_tune));

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

  const float crossMod = apvts.getRawParameterValue("crossMod")->load();
  waveGenerator_.set_cross_mod(crossMod);
  if (crossMod > 0.f) {
    // todo: when turning crossmod back down the pitch mod gets "stuck"
    // minblep AA is not compatible with FM
    // todo: is this really true? I think there is some other issue...
    waveGenerator_.set_mode(WaveGenerator::NO_ANTIALIAS);
    wave2Generator_.set_mode(WaveGenerator::NO_ANTIALIAS);
  } else {
    waveGenerator_.set_mode(WaveGenerator::ANTIALIAS);
    wave2Generator_.set_mode(WaveGenerator::ANTIALIAS);
  }

  const double vco1Level =
      static_cast<double>(apvts.getRawParameterValue("vco1Level")->load());
  waveGenerator_.set_gain(vco1Level);
  const double vco2Level =
      static_cast<double>(apvts.getRawParameterValue("vco2Level")->load());
  wave2Generator_.set_gain(vco2Level);

  // filter
  const int filterEnvSource =
      static_cast<int>(apvts.getRawParameterValue("filterEnvSource")->load());
  if (filterEnvSource == 0) {
    filter_env_buffer_ = &env1_buffer_;
  } else {
    filter_env_buffer_ = &env2_buffer_;
  }
}

void OscillatorVoice::SetBlockSize(const int blockSize) {
  downsampler_.prepare(blockSize, kOversample);
  const auto oversample_samples = blockSize * kOversample;
  oversample_buffer_.setSize(1, oversample_samples, false, true);
  wave2_buffer_.setSize(1, oversample_samples, false, true);
  env1_buffer_.setSize(1, blockSize, false, true);
  env2_buffer_.setSize(1, blockSize, false, true);
}

void OscillatorVoice::startNote(const int midiNoteNumber,
                                [[maybe_unused]] const float velocity,
                                [[maybe_unused]] juce::SynthesiserSound* sound,
                                [[maybe_unused]] int pitchWheelPos) {
  waveGenerator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  wave2Generator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  envelope_.NoteOn();
  envelope2_.NoteOn();
}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                               [[maybe_unused]] const bool allowTailOff) {
  envelope_.NoteOff();
  envelope2_.NoteOff();
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {
}

void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                      [[maybe_unused]] int newControllerValue) {
}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      const int numSamples) {
  const auto oversample_samples = numSamples * kOversample;;
  const auto oversample_start_sample = startSample * kOversample;

  // TODO: how does this interact with note on? Does this mean envelope always
  //  starts at start of a block even if it "should" start mid-block?
  // fill envelope buffers
  envelope_.WriteEnvelopeToBuffer(env1_buffer_, startSample, numSamples);
  envelope2_.WriteEnvelopeToBuffer(env2_buffer_, startSample, numSamples);

  // note this will fill and process only the left channel since we want to work
  // in mono until the last moment the wave generator and filter are already
  // configured to generate at 2x oversampling..
  // we need wave2 first so we can use it for cross-mod (FM)
  // TODO: should the envelope actually affect the cross-mod behavior?
  // todo: add ability to tell RenderNextBlock to OVERWRITE instead of add
  //    I think we need this clear since wave generator will ADD so we can avoid the clearing
  wave2_buffer_.clear(oversample_start_sample, oversample_samples);
  wave2Generator_.RenderNextBlock(wave2_buffer_, oversample_start_sample, oversample_samples);
  oversample_buffer_.clear( oversample_start_sample, oversample_samples);
  // todo: Do we even need this intermediate wave2_buffer? What if we cross-mod
  // from the oversample_buffer_ directly? if we're doing FM, we only use wave 2
  // for FM, we don't output it directly todo: this should be a bool set in
  // Configure, not doing this check every block...
   if (waveGenerator_.cross_mod() <= 0.0001f) {
    oversample_buffer_.addFrom(0, oversample_start_sample, wave2_buffer_, 0, oversample_start_sample, oversample_samples);
  }
  waveGenerator_.RenderNextBlock(oversample_buffer_, oversample_start_sample, oversample_samples);

  filter_.Process(oversample_buffer_, *filter_env_buffer_,
                  waveGenerator_.lfo_buffer(), oversample_start_sample, oversample_samples);

  DetectClip(oversample_buffer_, "post filter");

  // Apply ADSR envelope to the mono oversampled buffer (VCA)
  auto* data = oversample_buffer_.getWritePointer(0);
  auto* env1_data = env1_buffer_.getReadPointer(0);
  for (int i = oversample_start_sample; i < oversample_samples; ++i) {
    data[i] *= env1_data[i / kOversample];
  }

  // clipping check
  DetectClip(oversample_buffer_, "post VCA env");

  if (!envelope_.IsActive()) {
    // todo: might need this or no?
    //waveGenerator_.set_volume(-120);
    //wave2Generator_.set_volume(-120);
    clearCurrentNote();
  }

  downsampler_.process(oversample_buffer_, outputBuffer, oversample_start_sample, oversample_samples);

  // post-downsample clipping check
  DetectClip(outputBuffer, "post downsample");
}
}  // namespace audio_plugin