/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/WaveGenerator.h
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <BBSynth/MinBlepGenerator.h>

namespace audio_plugin {
// ==== SIMPLER LFO - non AP
// ==========================================================================
class WaveGenerator {
  MinBlepGenerator myBlepGenerator;

  double blepOvertoneDepth = 1;  // tweak the "resolution" of the blep ...
                                 // (multiple of highest harmonics allowed)

  // MASTER - HARD SYNC ::::
  double masterDeltaBase = 0;    // for hard syncing
  double masterPitchOffset = 1;  // relative offset ....
  double currentMasterAngle = 0;

  // SLAVE - HARD SYNC ::::
  double slaveDeltaBase = 0;
  double slavePitchOffset = 1;  // relative offset ....
  double currentAngle = 0;
  double currentAngleSkewed = 0;
  double lastAngleSkewed = 0;

  // ACTUAL OUTPUT
  double lastSample = 0;
  double lastSampleDelta = 0;

  // PITCH BEND
  double pitchBendTarget = 0;
  double pitchBendActual = 0;

  // ACTUAL delta
  // - takes into account pitch bend and phase shift ...
  double actualCurrentAngleDelta = 0;  // ACTUAL CURRENT DELTA (pitch)

  double volume = 1;
  double pan = 0.5;             // [0..1]
  double gainLast[2] = {0, 0};  // for ramping ..
  double skew = 0;              // [-1, 1]
  double sampleRate = 0;

  juce::Array<float> wave;  // hmmm ... faster to make this a pointer I bet ....
  juce::Array<float>
      history;  // a running averaged wave, for rendering purposes
  int historyLength = 500;

  bool stereo = false;  // invert the right channel ...
  double phaseAngleTarget = 0;
  double phaseAngleActual =
      0;  // the target angle to get to (used for phase shifting)

  bool hardSync = true;

public:
  static void setTest(double newTest);
  static void setClear(bool clear);

  enum WaveType {

    sine = 0,
    sawRise = 1,
    sawFall = 2,
    triangle = 3,
    square = 4,
    random = 5
  };
  WaveType myWaveType;

  // these can be used to generate LFO waves
  // or high speed (synth) waveforms ...
  enum WaveMode {

    ANTIALIAS,
    BUILD_AA,
    NO_ANTIALIAS

  };

  WaveMode myMode;

  WaveGenerator();

  void prepareToPlay(double newSampleRate);

  void setWaveType(WaveType newWaveType) {
    myWaveType = newWaveType;

    if (myWaveType == triangle)
      myBlepGenerator.setToReturnDerivative(true);
    else if (myWaveType == sine)
      myBlepGenerator.setToReturnDerivative(true);
    else
      myBlepGenerator.setToReturnDerivative(false);
  }

  WaveType getWaveType() const { return myWaveType; }

  void setMode(WaveMode mode) {
    myMode = mode;
    // BUILD the appropriate BLEP step ....
    if (myMode == ANTIALIAS) {
      myBlepGenerator.buildBlep();
    }
  }

  WaveMode getAntialiasMode() const { return myMode; }

  // minBLEP :::
  void setBlepOvertoneDepth(double newMult);
  MinBlepGenerator* getBlepGenerator() { return &myBlepGenerator; }
  void changeBlepSize(float sampleRateForBlep);

  double getSlaveDeltaBase() const { return slaveDeltaBase; }
  double getMasterDeltaBase() const { return masterDeltaBase; }
  double getAngleDeltaActual() const { return actualCurrentAngleDelta; }

  void setMasterDelta(double newAngleDelta) {
    // MASTER OSC ::::
    masterDeltaBase = masterPitchOffset * newAngleDelta;

    // SLAVE OSC ::::
    slaveDeltaBase = slavePitchOffset * newAngleDelta;
  }
  double getCurrentAngle() const { return currentAngle; }

  void setPitchSemitone(const int midiNoteValue, const double pSampleRate) {
    double centerF =
        juce::MidiMessage::getMidiNoteInHertz(midiNoteValue);  // ....
    double cyclesPerSample = centerF / pSampleRate;
    float angleDelta = static_cast<float>(cyclesPerSample * 2.0 *
                                          juce::MathConstants<double>::twoPi);

    setMasterDelta(static_cast<double>(angleDelta));
  }

  void setPitchHz(double newFreq) {
    double cyclesPerSample = newFreq / sampleRate;
    float angleDelta = static_cast<float>(cyclesPerSample * 2.0 *
                                          juce::MathConstants<double>::twoPi);
    setMasterDelta(static_cast<double>(angleDelta));
  }

  double getCurrentPitchHz() const {
    // float angleDelta = cyclesPerSample * 2.0 * double_Pi;
    double cyclesPerSample =
        actualCurrentAngleDelta / (2.0 * juce::MathConstants<double>::twoPi);
    double Freq = cyclesPerSample * sampleRate;

    return Freq;
  }

  // todo no impl - intentional?
  juce::Array<float> getBLEPArray();

  float getSkew() const { return static_cast<float>(skew); }
  void setSkew(double newSkew) {
    jassert(newSkew >= -1 && newSkew <= 1);
    skew = newSkew;
  }

  double getPhaseTarget() const { return phaseAngleTarget; }
  void setPhaseTarget(double angleToGetTo) {
    jassert(angleToGetTo >= -juce::MathConstants<double>::twoPi &&
            angleToGetTo <= juce::MathConstants<double>::twoPi);
    phaseAngleTarget = angleToGetTo;
  }

  // PITCH MOD ::: shifts the MASTER angleDelta up/down in semitones ...
  void setPitchOffset(double pitchOffsetInSemitones) {
    // TONE is ALWAYS higher than master center (or has no effect)
    double slaveToneOffset = getToneOffsetInSemis();

    // Convert from semitones to * factor
    masterPitchOffset = (pow(2, pitchOffsetInSemitones / 12.));

    // UPDATE the slave freq
    double newToneOffset = pitchOffsetInSemitones + slaveToneOffset;
    slavePitchOffset = (pow(2, newToneOffset / 12));
  }
  double getPitchOffsetInSemis() const {
    // return the Log pitch offset ....
    double pitchOffsetInSemis = 12 * log2(masterPitchOffset);
    return pitchOffsetInSemis;
  }

  void setToneOffset(const double newToneOffsetInSemis) {
    // Convert from semitones to * factor
    const double masterSemis = getPitchOffsetInSemis();

    // TONE is ALWAYS higher than master center (or has no effect)
    const double newToneOffset = masterSemis + newToneOffsetInSemis;
    slavePitchOffset = (pow(2, newToneOffset / 12.));
  }
  double getToneOffsetInSemis() const {
    // return the Log pitch offset ....
    const double toneOffsetInSemis = 12 * log2(slavePitchOffset);

    // RELATIVE to master ...
    const double masterSemis = getPitchOffsetInSemis();

    return (toneOffsetInSemis - masterSemis);
  }

  void setPitchBend(const double newBendInSemiTones) {
    jassert(newBendInSemiTones == newBendInSemiTones);

    // For EQUAL TEMPERMENT :::::
    pitchBendTarget = pow(2.0, (newBendInSemiTones / 12.0));
  }

  double getPitchBendSemis() const { return 12 * log2(pitchBendActual); }

  void setVolume(const double dBMult) {
    if (dBMult <= -80)
      volume = 0;
    else
      volume = juce::Decibels::decibelsToGain(dBMult);
  }
  void setPan(const double proportionalPan) {
    jassert(proportionalPan >= 0);
    jassert(proportionalPan <= 1);
    pan = proportionalPan;
  }
  void setStereo(const bool isStereo) { stereo = isStereo; }
  void setHardsync(const bool shouldHardSync) { hardSync = shouldHardSync; }

  bool isStereo() const { return stereo; }
  bool isHardSync() const { return hardSync; }

  juce::Array<float> getHistory() { return history; }

  void clear();

  // FAST RENDER (AP) :::::
  void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int numSamples);
  inline void buildWave(int numSamples);

  // SLOW RENDER (LFO) ::::::
  void moveAngleForward(int numSamples);
  void moveAngleForwardTo(double newAngle);

  double getAngleAfter(double samplesSinceRollover) {
    // CALCULATE the WAVEFORM'S ANGULAR OFFSET
    // GIVEN a certain number of samples (since rollover) ....

    // since this is only done in LFO mode ....
    // reset phase so there is no changing ...
    phaseAngleActual = phaseAngleTarget;

    return slaveDeltaBase * samplesSinceRollover + phaseAngleActual;
  }
  double skewAngle(double angle) const;

  // GET value
  double getCurrentValue() {
    // SKEW the "current angle"
    const double skewedAngle = skewAngle(currentAngle);
    return getValueAt(skewedAngle);
  }
  double getValueAt(double angle);

  // Wave calculations ...
  static inline double getSine(double angle) {
    const double sample = sin(angle);
    return sample;
  }

  static inline double getSawRise(const double angle) {
    // remainder ....
    double sample = getSawFall(angle);

    // JUST INVERT IT NOW ... to get rising ...
    sample = -sample;

    return sample;
  }

  static inline double getSawFall(double angle) {
    angle = fmod(angle + juce::MathConstants<double>::twoPi,
                 2 * juce::MathConstants<double>::twoPi);  // shift x
    const double sample = angle / juce::MathConstants<double>::twoPi -
                          1;  // computer as remainder

    return sample;
  }
  static inline double getTriangle(double angle) {
    double sample = 0;

    // using a simple offset, we can make this a ramp up, then down ....
    // angle += double_Pi/4;
    angle = fmod(angle + juce::MathConstants<double>::twoPi / 2,
                 2 * juce::MathConstants<double>::twoPi);  // ROLL

    if (const double frac = angle / (2 * juce::MathConstants<double>::twoPi);
        frac < .5)
      sample = 2 * frac;  // RAMPS up
    else
      sample = 1 - 2 * (frac - .5);  // RAMPS down

    // SCALE and Y-OFFSET
    sample = (sample - .5) * 2;  // so it goes from -1 .. 1

    jassert(sample <= 1);
    jassert(sample >= -1);

    return sample;
  }

  static inline double getSquare(const double angle) {
    if (angle >= juce::MathConstants<double>::twoPi)
      return -1;
    return 1;
  }

  inline double getRandom([[maybe_unused]] double angle) {
    double r = static_cast<double>(juce::Random::getSystemRandom().nextFloat());

    r = 2 * (r - 0.5);  // scale to -1 .. 1
    r = juce::jlimit(-10 * slaveDeltaBase, 10 * slaveDeltaBase, r);

    lastSample += r;

    // limit it based on the F ...
    lastSample = juce::jlimit(-1.0, 1.0, lastSample);

    return lastSample;
  }

  // LOAD / SAVE ::::
  void loadScene(const juce::ValueTree& node);
  void saveScene(juce::ValueTree node) const;
};
}  // namespace audio_plugin