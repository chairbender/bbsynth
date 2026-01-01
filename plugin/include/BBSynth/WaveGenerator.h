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

namespace audio_plugin {

inline double GetSine(double angle);
inline double GetSawRise(double angle);
inline double GetSawFall(double angle);
inline double GetTriangle(double angle);
inline double GetSquare(double angle, double pulse_width = 0.5);

enum PulseWidthModType {
  env2Minus = 0,
  env2Plus = 1,
  env1Minus = 2,
  env1Plus = 3,
  lfo = 4,
  manual = 5
};

/**
*  SET the allowed depth of overtones
   ** NOTE **
   this is relative to the fundamental F
   so each factor of 2 is an octave.
   This default of 128 seems fairly high.
 */
constexpr auto kBlepOvertoneDepth = 128;

class WaveGenerator {
  // TODO: refactor to properly keep things private and put logic into separate
  // rather than header.

 public:
  enum WaveType {
    sine = 0,
    sawRise = 1,
    sawFall = 2,
    triangle = 3,
    square = 4,
    random = 5
  };
  enum HardSyncMode { PRIMARY = 0, SECONDARY = 1, DISABLED = 2 };

  enum WaveMode { ANTIALIAS, BUILD_AA, NO_ANTIALIAS };

 private:
  MinBlepGenerator blep_generator_;

  /**
   * Base phase increment (radians per sample) for this oscillator.
   */
  double delta_base_ = 0;

  /**
   * Relative pitch offset multiplier for this oscillator.
   */
  double pitch_offset_ = 1;

  /**
   * Current phase angle (0 to 2*PI) of this oscillator.
   * This is the angle used to generate the waveform samples.
   */
  double current_angle_ = 0;
  double current_angle_skewed_ = 0;
  double last_angle_skewed_ = 0;

  PulseWidthModType pulse_width_mod_type_ = manual;
  // different effects depending on mod type, but range should be 0. to 1.f
  double pulse_width_mod_ = 0.5;
  // actual current pwm for cur sample
  double pulse_width_actual_ = 0.5;

  double cross_mod_ = 0;

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
  // todo: what's the point of this though? It doesn't factor in skew
  double actual_current_angle_delta_ = 0;  // ACTUAL CURRENT DELTA (pitch)

  // todo: not used currently
  double volume_ = 1;
  double gain_last_[2] = {0, 0};  // for ramping ..

  double skew_ = 0;  // [-1, 1]
  double sample_rate_ = 0;

  juce::Array<float> wave;  // hmmm ... faster to make this a pointer I bet ....
  juce::Array<float>
      history_;  // a running averaged wave, for rendering purposes
  int history_length_ = 500;

  double phase_angle_target_ = 0;
  double phase_angle_actual_ =
      0;  // the target angle to get to (used for phase shifting)

  const juce::AudioBuffer<float>& lfo_buffer_;
  const juce::AudioBuffer<float>& env1_buffer_;
  const juce::AudioBuffer<float>& env2_buffer_;
  const juce::AudioBuffer<float>& modulator_buffer_;
  // see same field name on Oscillator
  juce::Array<float, juce::CriticalSection>& hard_sync_reset_sample_indices_;

  // what role is this oscillator serving in hard sync?
  HardSyncMode hard_sync_mode_ = DISABLED;

  WaveType wave_type_;
  WaveMode mode_;

 public:
  WaveGenerator(const juce::AudioBuffer<float>& lfo_buffer,
                const juce::AudioBuffer<float>& env1_buffer,
                const juce::AudioBuffer<float>& env2_buffer,
                const juce::AudioBuffer<float>& modulator_buffer,
                juce::Array<float, juce::CriticalSection>&
                    hard_sync_reset_sample_indices);

  void PrepareToPlay(double new_sample_rate);

  double cross_mod();
  void set_hard_sync_mode(HardSyncMode mode);
  // Enable/disable the post-BLEP DC blocker (1st-order high-pass) used in
  // ANTIALIAS mode
  void set_dc_blocker_enabled(bool enabled);
  void set_wave_type(WaveType wave_type);
  void set_mode(WaveMode mode);
  MinBlepGenerator* blep_generator();
  void set_pulse_width_mod_type(PulseWidthModType type);
  void set_pulse_width_mod(double pulse_width);

  /**
   * Set the delta base (phase increment in radians per sample)
   * for this oscillator
   */
  void set_delta_base(double radians);
  void set_pitch_semitone(int midi_note_value, double sample_rate);
  void set_pitch_hz(double freq);
  double current_pitch_hz() const;

  // PITCH MOD ::: shifts the primary angleDelta up/down in semitones ...
  void set_pitch_offset_semis(const double pitch_offset_in_semitones);
  void set_pitch_offset_hz(const double pitch_offset_in_hz);
  double pitch_offset_in_semis() const;
  void set_tone_offset(double new_tone_offset_in_semis);
  double tone_offset_in_semis() const;
  void set_pitch_bend(double newBendInSemiTones);
  double get_pitch_bend_semis() const;

  void set_volume(double db_mult);
  void set_gain(double gain);
  void set_cross_mod(float cross_mod);

  juce::Array<float> history();

  void clear();

  // FAST RENDER (AP) :::::
  /**
   * Fill the first channel of the buffer up to numSamples.
   */
  // todo: passing LFO like this is stupid, let's find a better way
  void RenderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample,
                       int numSamples);
  inline void BuildWave(int numSamples);

  void MoveAngleForward(int numSamples);
  void MoveAngleForwardTo(double newAngle);
  double GetAngleAfter(double samples_since_rollover);
  double skew_angle(double angle) const;

  double GetValueAt(double angle);
  void set_pitch_bend_lfo_mod(float mod);
  void set_pitch_bend_env1_mod(float mod);

  inline double GetRandom([[maybe_unused]] double angle);
};
}  // namespace audio_plugin