#include "BBSynth/Oscillator.h"

namespace audio_plugin {


OscillatorSound::OscillatorSound([[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) :
  centOffset_{nullptr} {
}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice() {
  waveGenerator_.prepareToPlay(getSampleRate());
  //waveGenerator_.setHardsync(false);
  waveGenerator_.setMode(WaveGenerator::ANTIALIAS);
  waveGenerator_.setWaveType(WaveGenerator::sawFall);
  filter_.set_sample_rate(getSampleRate());
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
  waveGenerator_.setPitchSemitone(midiNoteNumber, getSampleRate());
  waveGenerator_.setVolume(0);

}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                            [[maybe_unused]] const bool allowTailOff) {
  waveGenerator_.setVolume(-120);
  clearCurrentNote();
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {}
void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                    [[maybe_unused]] int newControllerValue) {}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      int numSamples) {
  // todo: oversampling

  // note this will fill and process only the left channel since we want to work in mono
  // until the last moment
  waveGenerator_.RenderNextBlock(outputBuffer, numSamples);
  filter_.Process(outputBuffer, numSamples);

  // todo: mono to stereo
}
}  // namespace audio_plugin
