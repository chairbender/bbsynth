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
  waveGenerator_.setVolume(-48);

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
  // todo can't be right? It needs to know startsample?
  // waveGenerator_.renderNextBlock(outputBuffer, numSamples);

  // todo: is this really right? why the buffer copying?
  juce::AudioSampleBuffer nextBuffer(2, numSamples);
  nextBuffer.clear();

  waveGenerator_.renderNextBlock(nextBuffer, numSamples);

  outputBuffer.copyFrom(0, startSample, *nextBuffer.getArrayOfReadPointers(), numSamples);
  outputBuffer.copyFrom(1, startSample, *nextBuffer.getArrayOfReadPointers(), numSamples);
}

SineWaveSound::SineWaveSound(juce::AudioProcessorValueTreeState& apvts)
    : centOffset_{apvts.getRawParameterValue("centOffset")} {}

bool SineWaveSound::appliesToNote([[maybe_unused]] int midiNoteNumber) {
  return true;
}
bool SineWaveSound::appliesToChannel([[maybe_unused]] int midiChannelNumber) {
  return true;
}
std::atomic<float>* SineWaveSound::getCentOffset() const {
  return centOffset_;
}

SineWaveVoice::SineWaveVoice() {}

bool SineWaveVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<SineWaveSound*>(sound) != nullptr;
}

void SineWaveVoice::startNote(const int midiNoteNumber,
                              const float velocity,
                              [[maybe_unused]] juce::SynthesiserSound* sound,
                              [[maybe_unused]] int pitchWheelPos) {
  currentAngle = 0.0;
  level = static_cast<double>(velocity) * 0.15;
  tailOff = 0.0;

  if (auto sinSound = dynamic_cast<SineWaveSound*>(sound)) {
    auto cyclesPerSecond =
        juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) +
        static_cast<double>(sinSound->getCentOffset()->load());
    auto cyclesPerSample = cyclesPerSecond / getSampleRate();

    angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
  }
}

void SineWaveVoice::stopNote([[maybe_unused]] float velocity,
                             const bool allowTailOff) {
  if (allowTailOff) {
    if (tailOff == 0.0) {
      tailOff = 1.0;
    } else {
      clearCurrentNote();
      angleDelta = 0.0;
    }
  }
}

void SineWaveVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {}
void SineWaveVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                    [[maybe_unused]] int newControllerValue) {}

void SineWaveVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                    const int startSample,
                                    const int numSamples) {
  const auto endSampleIdx{startSample + numSamples};
  auto curSampleIdx = startSample;
  if (angleDelta != 0.0) {
    if (tailOff > 0.0) {
      while (curSampleIdx < endSampleIdx) {
        const float currentSample{
            static_cast<float>(std::sin(currentAngle) * level * tailOff)};
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
        const auto currentSample{
            static_cast<float>(std::sin(currentAngle) * level)};
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;) {
          outputBuffer.addSample(i, curSampleIdx, currentSample);
        }
        currentAngle += angleDelta;
        ++curSampleIdx;
      }
    }
  }
}

}  // namespace audio_plugin
