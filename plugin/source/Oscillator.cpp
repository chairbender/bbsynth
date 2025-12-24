#include "BBSynth/Oscillator.h"

namespace audio_plugin {

constexpr auto kOversample = 2;

OscillatorSound::OscillatorSound([[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) {}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice() {
  waveGenerator_.PrepareToPlay(getSampleRate()*kOversample);
  //waveGenerator_.setHardsync(false);
  waveGenerator_.set_mode(WaveGenerator::ANTIALIAS);
  waveGenerator_.set_wave_type(WaveGenerator::sawFall);
  filter_.set_sample_rate(getSampleRate()*kOversample);
}

bool OscillatorVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<OscillatorSound*>(sound) != nullptr;
}

void OscillatorVoice::Configure(const juce::AudioProcessorValueTreeState& apvts) {
  filter_.Configure(apvts);

  // Configure ADSR envelope from parameters
  envelope_.setSampleRate(getSampleRate() * kOversample);
  juce::ADSR::Parameters params{};
  if (auto* a = apvts.getRawParameterValue("adsrAttack")) params.attack = a->load();
  if (auto* d = apvts.getRawParameterValue("adsrDecay")) params.decay = d->load();
  if (auto* s = apvts.getRawParameterValue("adsrSustain")) params.sustain = s->load();
  if (auto* r = apvts.getRawParameterValue("adsrRelease")) params.release = r->load();
  envelope_.setParameters(params);
}

void OscillatorVoice::SetBlockSize(const int blockSize) {
  downsampler_.prepare(getSampleRate(), blockSize);
}

void OscillatorVoice::startNote(const int midiNoteNumber,
                                [[maybe_unused]] const float velocity,
                                [[maybe_unused]] juce::SynthesiserSound* sound,
                                [[maybe_unused]] int pitchWheelPos) {
  waveGenerator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  waveGenerator_.set_volume(0);
  envelope_.noteOn();
}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                            [[maybe_unused]] const bool allowTailOff) {
  waveGenerator_.set_volume(-120);
  if (allowTailOff) {
    envelope_.noteOff();
  } else {
    envelope_.reset();
    clearCurrentNote();
  }
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {}
void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                    [[maybe_unused]] int newControllerValue) {}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      const int numSamples) {
  const auto oversample_samples = numSamples*kOversample;
  oversample_buffer_.setSize(1, oversample_samples, false, true);

  // note this will fill and process only the left channel since we want to work in mono
  // until the last moment
  // the wave generator and filter are already configured to generate at 2x oversampling.
  waveGenerator_.RenderNextBlock(oversample_buffer_, oversample_samples);
  filter_.Process(oversample_buffer_, oversample_samples);

  // Apply ADSR envelope to the mono oversampled buffer (VCA)
  auto* data = oversample_buffer_.getWritePointer(0);
  for (int i = 0; i < oversample_samples; ++i) {
    data[i] *= envelope_.getNextSample();
  }

  downsampler_.process(oversample_buffer_, outputBuffer, numSamples);

  if (! envelope_.isActive()) {
    clearCurrentNote();
  }
}
}  // namespace audio_plugin
