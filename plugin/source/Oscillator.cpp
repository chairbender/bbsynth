#include "BBSynth/Oscillator.h"

namespace audio_plugin {


OscillatorSound::OscillatorSound([[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) {}

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
}

bool OscillatorVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<OscillatorSound*>(sound) != nullptr;
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
  // todo: is this really right? why the buffer copying?
  juce::AudioSampleBuffer nextBuffer(2, numSamples);
  nextBuffer.clear();

  waveGenerator_.renderNextBlock(nextBuffer, numSamples);

  outputBuffer.copyFrom(0, startSample, *nextBuffer.getArrayOfReadPointers(), numSamples);
  outputBuffer.copyFrom(1, startSample, *nextBuffer.getArrayOfReadPointers(), numSamples);
}
}  // namespace audio_plugin
