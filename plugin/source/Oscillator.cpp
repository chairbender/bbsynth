#include "BBSynth/Oscillator.h"

namespace audio_plugin {

// todo: oversampling
constexpr auto kOversample = 1;

OscillatorSound::OscillatorSound([[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) :
  centOffset_{nullptr} {
}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice() : oversampler_(1, 2,
  juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple) {
  waveGenerator_.PrepareToPlay(getSampleRate()*(kOversample));
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
}

void OscillatorVoice::startNote(const int midiNoteNumber,
                                [[maybe_unused]] const float velocity,
                                [[maybe_unused]] juce::SynthesiserSound* sound,
                                [[maybe_unused]] int pitchWheelPos) {
  waveGenerator_.set_pitch_semitone(midiNoteNumber, getSampleRate());
  waveGenerator_.set_volume(0);

}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                            [[maybe_unused]] const bool allowTailOff) {
  waveGenerator_.set_volume(-120);
  clearCurrentNote();
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {}
void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                    [[maybe_unused]] int newControllerValue) {}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      const int numSamples) {
  const auto oversample_samples = numSamples*kOversample;
  oversample_buffer_.setSize(1, oversample_samples, false, true);

  // todo probably shouldnt call each block
  //oversampler_.initProcessing(static_cast<size_t>(oversample_samples));

  // note this will fill and process only the left channel since we want to work in mono
  // until the last moment
  // todo: for oversampling, the blep approach needs to be reworked as it assumes a specific sample rate
  waveGenerator_.RenderNextBlock(oversample_buffer_, oversample_samples);
  filter_.Process(oversample_buffer_, oversample_samples);

  // todo: downsample
  //juce::dsp::AudioBlock<float> block(oversample_buffer_.getArrayOfWritePointers(), 1,
    //static_cast<size_t>(oversample_samples));
  //oversampler_.processSamplesDown(block);

  outputBuffer.copyFrom(0, 0,
    oversample_buffer_.getReadPointer(0), numSamples);

}
}  // namespace audio_plugin
