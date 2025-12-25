/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/WaveGenerator.h
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/
#pragma once

#include <BBSynth/MinBlepGenerator.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace audio_plugin {

enum PulseWidthModType {
  env2Minus = 0,
  env2Plus = 1,
  env1Minus = 2,
  env1Plus = 3,
  lfo = 4,
  manual = 5
};

class WaveGenerator {
  MinBlepGenerator blep_generator_;

  double blep_overtone_depth_ = 1;  // tweak the "resolution" of the blep ...
                                    // (multiple of highest harmonics allowed)

  // PRIMARY - HARD SYNC ::::
  double primary_delta_base_ = 0;    // for hard syncing
  double primary_pitch_offset_ = 1;  // relative offset ....
  double primary_angle_ = 0;

  PulseWidthModType pulse_width_mod_type_ = manual;
  // different effects depending on mod type, but range should be 0. to 1.f
  double pulse_width_mod_ = 0.5;
  // actual current pwm for cur sample
  double pulse_width_actual_ = 0.5;

  // secondary - HARD SYNC ::::
  double secondary_delta_base_ = 0;
  double secondary_pitch_offset_ = 1;  // relative offset ....
  double current_angle_ = 0;
  double current_angle_skewed_ = 0;
  double last_angle_skewed_ = 0;

  // ACTUAL OUTPUT (pre-AA)
  double last_sample_ = 0;
  double last_sample_delta_ = 0;

  // post-AA output (not defined unless AA enabled)
  double prev_buffer_last_sample_filtered_ = 0;
  double prev_buffer_last_sample_raw_ = 0;
  bool dc_blocker_enabled_ = true;

  // PITCH BEND
  double pitch_bend_target_ = 0;
  double pitch_bend_actual_ = 0;
  // amount of effect the LFO has on the pitch
  double pitch_bend_lfo_mod_ = 0;
  // amount of effect the env1 has on the pitch
  double pitch_bend_env1_mod_ = 0;

  // ACTUAL delta
  // - takes into account pitch bend and phase shift ...
  double actual_current_angle_delta_ = 0;  // ACTUAL CURRENT DELTA (pitch)

  double volume_ = 1;
  double gain_last_[2] = {0, 0};  // for ramping ..
  double skew_ = 0;               // [-1, 1]
  double sample_rate_ = 0;

  juce::Array<float> wave;  // hmmm ... faster to make this a pointer I bet ....
  juce::Array<float>
      history_;  // a running averaged wave, for rendering purposes
  int history_length_ = 500;

  double phase_angle_target_ = 0;
  double phase_angle_actual_ =
      0;  // the target angle to get to (used for phase shifting)

  bool hard_sync_ = true;

  const juce::AudioBuffer<float>& lfo_buffer_;
  const juce::AudioBuffer<float>& env1_buffer_;
  const juce::AudioBuffer<float>& env2_buffer_;

 public:
  enum WaveType {

    sine = 0,
    sawRise = 1,
    sawFall = 2,
    triangle = 3,
    square = 4,
    random = 5
  };
  WaveType wave_type_;

  // these can be used to generate LFO waves
  // or high speed (synth) waveforms ...
  enum WaveMode {

    ANTIALIAS,
    BUILD_AA,
    NO_ANTIALIAS

  };

  WaveMode mode_;

  WaveGenerator(const juce::AudioBuffer<float>& lfo_buffer,
                const juce::AudioBuffer<float>& env1_buffer,
                const juce::AudioBuffer<float>& env2_buffer);

  void PrepareToPlay(double new_sample_rate);

  // Enable/disable the post-BLEP DC blocker (1st-order high-pass) used in
  // ANTIALIAS mode
  void set_dc_blocker_enabled(bool enabled) { dc_blocker_enabled_ = enabled; }
  bool dc_blocker_enabled() const { return dc_blocker_enabled_; }

  void set_wave_type(WaveType wave_type) {
    wave_type_ = wave_type;

    if (wave_type_ == triangle)
      blep_generator_.set_return_derivative(true);
    else if (wave_type_ == sine)
      blep_generator_.set_return_derivative(true);
    else
      blep_generator_.set_return_derivative(false);
  }

  WaveType wave_type() const { return wave_type_; }

  void set_mode(const WaveMode mode) {
    mode_ = mode;
    // BUILD the appropriate BLEP step ....
    if (mode_ == ANTIALIAS) {
      blep_generator_.BuildBlep();
    }
  }

  WaveMode mode() const { return mode_; }

  // minBLEP :::
  void set_blep_overtone_depth(double mult);
  MinBlepGenerator* blep_generator() { return &blep_generator_; }
  void set_blep_size(float sample_rate_for_blep);

  void set_pulse_width_mod_type(const PulseWidthModType type) {
    pulse_width_mod_type_ = type;
  }

  void set_pulse_width_mod(const double pulse_width) {
    jassert(pulse_width >= 0 && pulse_width <= 1);
    pulse_width_mod_ = pulse_width;
  }

  double secondary_delta_base() const { return secondary_delta_base_; }
  double primary_delta_base() const { return primary_delta_base_; }
  double getAngleDeltaActual() const { return actual_current_angle_delta_; }

  void set_primary_delta(double newAngleDelta) {
    primary_delta_base_ = primary_pitch_offset_ * newAngleDelta;
    secondary_delta_base_ = secondary_pitch_offset_ * newAngleDelta;
  }
  double current_angle() const { return current_angle_; }

  void set_pitch_semitone(const int midi_note_value, const double sample_rate) {
    const double centerF =
        juce::MidiMessage::getMidiNoteInHertz(midi_note_value);  // ....
    const double cyclesPerSample = centerF / sample_rate;
    const float angleDelta = static_cast<float>(
        cyclesPerSample * 2.0 * juce::MathConstants<double>::twoPi);

    set_primary_delta(static_cast<double>(angleDelta));
  }

  void set_pitch_hz(double freq) {
    const double cyclesPerSample = freq / sample_rate_;
    const float angleDelta = static_cast<float>(
        cyclesPerSample * 2.0 * juce::MathConstants<double>::twoPi);
    set_primary_delta(static_cast<double>(angleDelta));
  }

  double current_pitch_hz() const {
    // float angleDelta = cyclesPerSample * 2.0 * double_Pi;
    const double cyclesPerSample = actual_current_angle_delta_ /
                                   (2.0 * juce::MathConstants<double>::twoPi);
    const double freq = cyclesPerSample * sample_rate_;

    return freq;
  }

  float skew() const { return static_cast<float>(skew_); }
  void set_skew(double skew) {
    jassert(skew >= -1 && skew <= 1);
    skew_ = skew;
  }

  double phase_target() const { return phase_angle_target_; }
  void set_phase_target(double angle_to_get_to) {
    jassert(angle_to_get_to >= -juce::MathConstants<double>::twoPi &&
            angle_to_get_to <= juce::MathConstants<double>::twoPi);
    phase_angle_target_ = angle_to_get_to;
  }

  // PITCH MOD ::: shifts the primary angleDelta up/down in semitones ...
  void set_pitch_offset_semis(const double pitch_offset_in_semitones) {
    // TONE is ALWAYS higher than primary center (or has no effect)
    double secondary_tone_offset = tone_offset_in_semis();

    // Convert from semitones to * factor
    primary_pitch_offset_ = (pow(2, pitch_offset_in_semitones / 12.));

    // UPDATE the secondary freq
    double newToneOffset = pitch_offset_in_semitones + secondary_tone_offset;
    secondary_pitch_offset_ = (pow(2, newToneOffset / 12));
  }

  void set_pitch_offset_hz(const double pitch_offset_in_hz) {
    // todo: idk what this should do
    // TONE is ALWAYS higher than primary center (or has no effect)
    // double secondary_tone_offset = tone_offset_in_semis();

    // todo: the scaling here is totally not right
    primary_pitch_offset_ = 1 + pitch_offset_in_hz * .1;

    // todo: idk what this should do
    // // UPDATE the secondary freq
    // double newToneOffset = pitch_offset_in_semitones + secondary_tone_offset;
    // secondary_pitch_offset_ = (pow(2, newToneOffset / 12));
  }

  double pitch_offset_in_semis() const {
    // return the Log pitch offset ....
    double pitchOffsetInSemis = 12 * log2(primary_pitch_offset_);
    return pitchOffsetInSemis;
  }

  void set_tone_offset(const double new_tone_offset_in_semis) {
    // Convert from semitones to * factor
    const double primary_semis = pitch_offset_in_semis();

    // TONE is ALWAYS higher than primary center (or has no effect)
    const double newToneOffset = primary_semis + new_tone_offset_in_semis;
    secondary_pitch_offset_ = (pow(2, newToneOffset / 12.));
  }
  double tone_offset_in_semis() const {
    // return the Log pitch offset ....
    const double toneOffsetInSemis = 12 * log2(secondary_pitch_offset_);

    // RELATIVE to primary ...
    const double primary_semis = pitch_offset_in_semis();

    return (toneOffsetInSemis - primary_semis);
  }

  void set_pitch_bend(const double newBendInSemiTones) {
    jassert(newBendInSemiTones == newBendInSemiTones);

    // For EQUAL TEMPERMENT :::::
    pitch_bend_target_ = pow(2.0, (newBendInSemiTones / 12.0));
  }

  double get_pitch_bend_semis() const { return 12 * log2(pitch_bend_actual_); }

  void set_volume(const double db_mult) {
    if (db_mult <= -80)
      volume_ = 0;
    else
      volume_ = juce::Decibels::decibelsToGain(db_mult);
  }
  void set_hardsync(const bool shouldHardSync) { hard_sync_ = shouldHardSync; }
  bool hardsync() const { return hard_sync_; }

  juce::Array<float> history() { return history_; }

  void clear();

  // FAST RENDER (AP) :::::
  /**
   * Fill the first channel of the buffer up to numSamples.
   */
  // todo: passing LFO like this is stupid, let's find a better way
  void RenderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample,
                       int numSamples);
  inline void BuildWave(int numSamples);

  // SLOW RENDER (LFO) ::::::
  void MoveAngleForward(int numSamples);
  void MoveAngleForwardTo(double newAngle);

  double GetAngleAfter(double samplesSinceRollover) {
    // CALCULATE the WAVEFORM'S ANGULAR OFFSET
    // GIVEN a certain number of samples (since rollover) ....

    // since this is only done in LFO mode ....
    // reset phase so there is no changing ...
    phase_angle_actual_ = phase_angle_target_;

    return secondary_delta_base_ * samplesSinceRollover + phase_angle_actual_;
  }
  double skew_angle(double angle) const;

  // GET value
  double current_value() {
    // SKEW the "current angle"
    const double skewedAngle = skew_angle(current_angle_);
    return GetValueAt(skewedAngle);
  }
  double GetValueAt(double angle);
  void set_pitch_bend_lfo_mod(float mod);
  void set_pitch_bend_env1_mod(float mod);

  static inline double GetSine(double angle) {
    const double sample = sin(angle);
    return sample;
  }

  static double GetSawRise(const double angle) {
    // remainder ....
    double sample = GetSawFall(angle);

    // JUST INVERT IT NOW ... to get rising ...
    sample = -sample;

    return sample;
  }

  static double GetSawFall(double angle) {
    angle = fmod(angle + juce::MathConstants<double>::twoPi,
                 2 * juce::MathConstants<double>::twoPi);  // shift x
    const double sample = angle / juce::MathConstants<double>::twoPi -
                          1;  // computer as remainder

    return sample;
  }
  static double GetTriangle(double angle) {
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

  static inline double GetSquare(const double angle,
                                 const double pulse_width = 0.5) {
    if (angle >= juce::MathConstants<double>::twoPi * pulse_width) return -1;
    return 1;
  }

  inline double GetRandom([[maybe_unused]] double angle) {
    double r = static_cast<double>(juce::Random::getSystemRandom().nextFloat());

    r = 2 * (r - 0.5);  // scale to -1 .. 1
    r = juce::jlimit(-10 * secondary_delta_base_, 10 * secondary_delta_base_,
                     r);

    last_sample_ += r;

    // limit it based on the F ...
    last_sample_ = juce::jlimit(-1.0, 1.0, last_sample_);

    return last_sample_;
  }

  // LOAD / SAVE ::::
  void LoadScene(const juce::ValueTree& node);
  void SaveScene(juce::ValueTree node) const;
};
}  // namespace audio_plugin