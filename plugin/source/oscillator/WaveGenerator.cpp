/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/WaveGenerator.cpp
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/

// todo restore == checks against the same var as isNaN checks as that was the
// actual intention

#include "WaveGenerator.h"

#include "../Constants.h"

namespace audio_plugin {
constexpr double DELTA{.0000001};

inline double GetSine(const double angle) {
  const double sample = sin(angle);
  return sample;
}

inline double GetSawRise(const double angle) {
  // remainder ....
  double sample = GetSawFall(angle);

  // JUST INVERT IT NOW ... to get rising ...
  sample = -sample;

  return sample;
}

inline double GetSawFall(double angle) {
  angle = fmod(angle + juce::MathConstants<double>::twoPi,
               2 * juce::MathConstants<double>::twoPi);  // shift x
  const double sample =
      angle / juce::MathConstants<double>::twoPi - 1;  // computer as remainder

  return sample;
}
inline double GetTriangle(double angle) {
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

inline double GetSquare(const double angle, const double pulse_width) {
  if (angle >= juce::MathConstants<double>::twoPi * pulse_width) return -1;
  return 1;
}

template <bool IsLFO>
WaveGenerator<IsLFO>::WaveGenerator(
    const juce::AudioBuffer<float>& lfo_buffer, const juce::AudioBuffer<float>& env1_buffer,
    const juce::AudioBuffer<float>& env2_buffer,
    const juce::AudioBuffer<float>& modulator_buffer,
    juce::Array<float>& hard_sync_reset_sample_indices)
  requires(!IsLFO)
    : lfo_buffer_{lfo_buffer},
      env1_buffer_(env1_buffer),
      env2_buffer_(env2_buffer),
      modulator_buffer_{modulator_buffer},
      hard_sync_reset_sample_indices_{hard_sync_reset_sample_indices} {
  history_length_ = 500;
  sample_rate_ = 0;

  mode_ = NO_ANTIALIAS;
  wave_type_ = square;

  delta_base_ = 0;
  actual_current_angle_delta_ = 0;

  current_angle_ = 0.01;  // needed
  current_angle_skewed_ = last_angle_skewed_ = 0;
  pitch_bend_target_ = pitch_bend_actual_ = 1.0;

  pitch_offset_ = 1;

  volume_ = 1;
  gain_last_[0] = gain_last_[1] = 0;
  skew_ = 0;
  last_sample_ = 0;

  for (int i = 0; i < history_length_; i++) history_.add(0);

  phase_angle_target_ = phase_angle_actual_ = 0;  // expressed 0 - 2*PI
}

// todo: dedupe with above
template <bool IsLFO>
WaveGenerator<IsLFO>::WaveGenerator()
  requires IsLFO
{
  history_length_ = 500;
  sample_rate_ = 0;

  mode_ = NO_ANTIALIAS;
  wave_type_ = square;

  delta_base_ = 0;
  actual_current_angle_delta_ = 0;

  current_angle_ = 0.01;  // needed
  current_angle_skewed_ = last_angle_skewed_ = 0;
  pitch_bend_target_ = pitch_bend_actual_ = 1.0;

  pitch_offset_ = 1;

  volume_ = 1;
  gain_last_[0] = gain_last_[1] = 0;
  skew_ = 0;
  last_sample_ = 0;

  for (int i = 0; i < history_length_; i++) history_.add(0);

  phase_angle_target_ = phase_angle_actual_ = 0;  // expressed 0 - 2*PI
}

// Enable/disable the post-BLEP DC blocker (1st-order high-pass) used in
// ANTIALIAS mode
template <bool IsLFO>
void WaveGenerator<IsLFO>::set_dc_blocker_enabled(const bool enabled) {
  dc_blocker_enabled_ = enabled;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_wave_type(const WaveType wave_type) {
  wave_type_ = wave_type;

  if (wave_type_ == triangle)
    blep_generator_.set_return_derivative(true);
  else if (wave_type_ == sine)
    blep_generator_.set_return_derivative(true);
  else
    blep_generator_.set_return_derivative(false);
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_mode(const WaveMode mode) {
  mode_ = mode;
  // BUILD the appropriate BLEP step ....
  if (mode_ == ANTIALIAS) {
    blep_generator_.BuildBlep();
  }
}

template <bool IsLFO>
MinBlepGenerator* WaveGenerator<IsLFO>::blep_generator() {
  return &blep_generator_;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pulse_width_mod_type(
    const PulseWidthModType type) {
  pulse_width_mod_type_ = type;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pulse_width_mod(const double pulse_width) {
  jassert(pulse_width >= 0 && pulse_width <= 1);
  pulse_width_mod_ = pulse_width;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_delta_base(const double radians) {
  delta_base_ = pitch_offset_ * radians;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_semitone(const int midi_note_value,
                                              const double sample_rate) {
  const double centerF = juce::MidiMessage::getMidiNoteInHertz(midi_note_value);
  const double cyclesPerSample = centerF / sample_rate;
  const float angleDelta = static_cast<float>(
      cyclesPerSample * 2.0 * juce::MathConstants<double>::twoPi);

  set_delta_base(static_cast<double>(angleDelta));
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_hz(const double freq) {
  const double cyclesPerSample = freq / sample_rate_;
  const float angleDelta = static_cast<float>(
      cyclesPerSample * 2.0 * juce::MathConstants<double>::twoPi);
  set_delta_base(static_cast<double>(angleDelta));
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::current_pitch_hz() const {
  // float angleDelta = cyclesPerSample * 2.0 * double_Pi;
  const double cyclesPerSample =
      actual_current_angle_delta_ / (2.0 * juce::MathConstants<double>::twoPi);
  const double freq = cyclesPerSample * sample_rate_;

  return freq;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_offset_semis(
    const double pitch_offset_in_semitones) {
  // TONE is ALWAYS higher than primary center (or has no effect)
  double secondary_tone_offset = tone_offset_in_semis();

  // UPDATE the secondary freq
  double newToneOffset = pitch_offset_in_semitones + secondary_tone_offset;
  pitch_offset_ = (pow(2, newToneOffset / 12));
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::pitch_offset_in_semis() const {
  // return the Log pitch offset ....
  double pitchOffsetInSemis = 12 * log2(pitch_offset_);
  return pitchOffsetInSemis;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_tone_offset(
    const double new_tone_offset_in_semis) {
  // Convert from semitones to * factor
  const double primary_semis = pitch_offset_in_semis();

  // TONE is ALWAYS higher than primary center (or has no effect)
  const double newToneOffset = primary_semis + new_tone_offset_in_semis;
  pitch_offset_ = (pow(2, newToneOffset / 12.));
}
template <bool IsLFO>
double WaveGenerator<IsLFO>::tone_offset_in_semis() const {
  // return the Log pitch offset ....
  const double toneOffsetInSemis = 12 * log2(pitch_offset_);

  // RELATIVE to primary ...
  const double primary_semis = pitch_offset_in_semis();

  return (toneOffsetInSemis - primary_semis);
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_bend(const double newBendInSemiTones) {
  jassert(newBendInSemiTones == newBendInSemiTones);

  // For EQUAL TEMPERMENT :::::
  pitch_bend_target_ = pow(2.0, (newBendInSemiTones / 12.0));
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::get_pitch_bend_semis() const {
  return 12 * log2(pitch_bend_actual_);
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_volume(const double db_mult) {
  if (db_mult <= -80)
    volume_ = 0;
  else
    volume_ = juce::Decibels::decibelsToGain(db_mult);
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_gain(const double gain) {
  volume_ = gain;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_cross_mod(const float cross_mod) {
  cross_mod_ = static_cast<double>(cross_mod);
}

template <bool IsLFO>
juce::Array<float> WaveGenerator<IsLFO>::history() {
  return history_;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::PrepareToPlay(double new_sample_rate) {
  sample_rate_ = new_sample_rate;

  // BUILD the appropriate BLEP step ....
  blep_generator_.BuildBlep();
}
template <bool IsLFO>
double WaveGenerator<IsLFO>::cross_mod() const {
  return cross_mod_;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_hard_sync_mode(const HardSyncMode mode) {
  hard_sync_mode_ = mode;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::clear() {
  current_angle_ = phase_angle_target_;  // + phase !!!

  pitch_bend_target_ = pitch_bend_actual_ = 1.0;
  delta_base_ = 0.0;
  gain_last_[0] = gain_last_[1] = 0;
}

// FAST RENDER (AP) :::::
template <bool IsLFO>
void WaveGenerator<IsLFO>::RenderNextBlock(
    juce::AudioBuffer<float>& outputBuffer, const int startSample,
    const int numSamples) {
  jassert(sample_rate_ != 0.);

  if (delta_base_ == 0.0) return;

  // todo FIX !!!!
  if (volume_ == 0. && gain_last_[0] == 0. && gain_last_[1] == 0. &&
      blep_generator_.IsClear())
    return;

  BuildWave(numSamples);

  // ADD BAND-LIMITED (minBLEP) transitions :::
  // LFO doesn't do blepping, so no need for this in such cases
  if constexpr (!IsLFO) {
    if (mode_ == ANTIALIAS) {
      // Since we KNOW the intended F ... relative to F(sampling)
      // We can tweak the minBLEP to limit any harmonic above 4*(desired F)

      // TUNE the blep ....
      double freq = current_pitch_hz();               // Current, playing, Freq
      double relativeFreq = 2 * freq / sample_rate_;  // 2 for Nyquist ...
      relativeFreq *= kBlepOvertoneDepth;  // ie - up to the 3nd harmonic (2*2*2
      // -> 8x fundamental)

      blep_generator_.set_limiting_freq(
          static_cast<float>(relativeFreq));  // up to the 2nd harmonic ..
      blep_generator_.ProcessBlock(wave.getRawDataPointer(), numSamples);

      // dc blocker (1st-order high-pass): y[n] = x[n] - x[n-1] + R*y[n-1]
      // this is needed because (afaict) a properly-implemented minblep adds
      // a DC offset due to the effect of multiple bleps (which themselves
      // have a positive bias) adding together, which gets worse as the note
      // gets higher.
      if (dc_blocker_enabled_) {
        const auto samples = wave.getRawDataPointer();

        // Preserve last raw input of this block (before we overwrite samples)
        const float lastInputRaw = samples[numSamples - 1];

        // Use persistent states from previous block
        auto xPrev = static_cast<float>(prev_buffer_last_sample_raw_);
        auto yPrev = static_cast<float>(prev_buffer_last_sample_filtered_);

        for (int i = 0; i < numSamples; ++i) {
          constexpr float r = 0.995f;
          const float x = samples[i];
          const float y = (x - xPrev) + r * yPrev;
          samples[i] = y;
          xPrev = x;
          yPrev = y;
        }

        // Update states for next block
        prev_buffer_last_sample_raw_ = static_cast<double>(lastInputRaw);
        prev_buffer_last_sample_filtered_ = static_cast<double>(yPrev);
      }
    }
  }

  // BUILD ::::
  for (int i = 0; i < numSamples; i = i + 20) {
    // just adding a sample every 20 or so to the history
    history_.add(wave.getUnchecked(i));

    if (history_.size() > history_length_) history_.remove(0);
  }

  // COPY it to the outputbuffer ....
  // todo do the gain staging more intelligently - I think one reason we need it
  //  is because the minblep can cause overshoots.
  //  but we shouldn't apply it to the lfo...
  //  athis is not really efficient way to do this.
  // this fixed amount helps to prevent clipping at this stage caused by
  // minblep-induced overshoots
  // + the combination of the 2 oscillators.
  // It is quite a large attenuation because the worst case is about a F0 saw
  // note,
  //  which produces a very loud blep
  const auto gain_stage = mode_ == ANTIALIAS ? .2f : 1.f;
  outputBuffer.addFromWithRamp(0, startSample, wave.getRawDataPointer(),
                               numSamples, gain_stage, gain_stage);

  // todo: we aren't using gain_last_ / volume right now
  gain_last_[0] = volume_;

#if JUCE_DEBUG

  // Using dB -12 to +3 ... normalized to 0..1 for indicator
  float RMS = outputBuffer.getRMSLevel(0, 0, outputBuffer.getNumSamples());

  // Aaron - wtf?  How do we get NaN .. but we do .... hmmmm
  if (isnanf(RMS)) {
    DBG("NaN " + juce::String(outputBuffer.getNumSamples()) + " " +
        juce::String(*outputBuffer.getReadPointer(0, 0)));
  }

  if (RMS > 5 || RMS < 0) {
    DBG("WOAH! " + juce::String(RMS) + " " +
        juce::String(*outputBuffer.getReadPointer(0, 0)));
  }

#endif
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::BuildWave(const int numSamples) {
  if (delta_base_ == 0.0) return;

  if (numSamples == 0) return;
  jassert(numSamples > 0);

  if (wave.size() != numSamples) wave.resize(numSamples);

  float* waveData = wave.getRawDataPointer();

  // LINEAR CHANGE for now .... over the numsamples (see above)
  double freqDelta = (pitch_bend_target_ - pitch_bend_actual_) /
                     static_cast<double>(numSamples);
  JUCE_SNAP_TO_ZERO(freqDelta);

  // SKEWING :::::
  // change the freqDelta - up to 2x the speed, when at PI, and corresponding
  // slowness at 0 ...
  double phaseShiftPerSample = 0;
  if (fabs(phase_angle_target_ - phase_angle_actual_) > DELTA) {
    // LINEAR RAMP from current phase to the target
    phaseShiftPerSample = (phase_angle_target_ - phase_angle_actual_) /
                          (2 * numSamples);  // 2 here is smoooothing ...
    phase_angle_actual_ =
        phase_angle_actual_ + phaseShiftPerSample * numSamples;
  }

  // BUILD ::::
  // LFO does not have per-sample modulations so we skip this in that case
  float next_hard_sync_reset_sample = -2.f;
  int hard_sync_reset_array_idx = -1;
  if constexpr (!IsLFO) {
    if (hard_sync_mode_ == PRIMARY) {
      hard_sync_reset_sample_indices_.clearQuick();
    } else if (hard_sync_mode_ == SECONDARY) {
      if (!hard_sync_reset_sample_indices_.isEmpty()) {
        hard_sync_reset_array_idx = 0;
        next_hard_sync_reset_sample = hard_sync_reset_sample_indices_[0];
      }
    }
  }

  std::conditional_t<IsLFO, std::monostate, const float*> lfo_data{};
  std::conditional_t<IsLFO, std::monostate, const float*> env1_data{};
  std::conditional_t<IsLFO, std::monostate, const float*> env2_data{};
  std::conditional_t<IsLFO, std::monostate, const float*> modulator_data{};
  if constexpr (!IsLFO) {
    lfo_data = lfo_buffer_.getReadPointer(0);
    env1_data = env1_buffer_.getReadPointer(0);
    env2_data = env1_buffer_.getReadPointer(0);
    modulator_data = modulator_buffer_.getReadPointer(0);
  }

  for (int i = 0; i < numSamples; i++) {
    // this seems to be used to prevent adding 2 bleps for one sample
    bool hard_sync_blep_occurred = false;

    // CHANGE the PITCH BEND (linear ramping)
    // TODO: manual pitch bend disabled currently
    // pitch_bend_actual_ += freqDelta;
    // TODO: account better for oversampling - this hardcoded amount isn't good
    if constexpr (!IsLFO) {
      double mod = 0;
      if (pitch_bend_lfo_mod_ != 0.) {
        mod = static_cast<double>(lfo_data[i / kOversample]) *
              pitch_bend_lfo_mod_;
      } else {
        mod = 0;
      }
      if (pitch_bend_env1_mod_ != 0.) {
        mod += static_cast<double>(env1_data[i / kOversample]) *
               pitch_bend_env1_mod_;
      }
      if (cross_mod_ > 0.001) {
        // unlike the other modulation buffers, the modulator is oversampled.
        // note blepping has already been applied to the modulator signal
        // so the carrier only needs to deal with its own discontinuities like
        // normal todo: is this reasoning actually correct for what blepping is
        // needed?
        // todo: this is not producing the expected sound...
        mod += static_cast<double>(modulator_data[i]) * cross_mod_;
      }
      pitch_bend_actual_ = 1 + mod;
    }

    if (fabs(pitch_bend_actual_ - 1) < .00001) pitch_bend_actual_ = 1;

    // FOR CALCULATIONS,
    // note the current, actual delta (base pitch modified by pitch bends and
    // phase shifting)
    actual_current_angle_delta_ =
        delta_base_ * pitch_bend_actual_ + phaseShiftPerSample;

    // LFO does not hard sync
    if constexpr (!IsLFO) {
      if (hard_sync_mode_ == SECONDARY) {
        // perform the reset if we just blepped
        // primary (unskewed) rollover
        // todo: currently ignoring what happens between start of this block
        //  and end of previous block
        if (next_hard_sync_reset_sample > -2.f && static_cast<float>(i) >= next_hard_sync_reset_sample) {
          // we will hard sync and generate a blep this sample

          // TODO: I'm not sure the hard sync bleps are anti-aliasing exactly right.
          //   Need to check more closely by making AA and oversampling toggle-able, and comparing behavior.
          // ADD the blep ...
          MinBlepGenerator::BlepOffset blep;
          blep.offset = static_cast<double>(-next_hard_sync_reset_sample);

          // CALCULATE the MAGNITUDE of ths 2nd ORDER (VEL) discontinuity
          // TRIG :: calculate the angle (rise/run) before and after the
          // rollover
          double delta = .0000001;  // MIN

          // what percent (0 to 1) into the sample did the reset occur at?
          // todo: at least I think that's what this was calculating
          const auto perc_before_roll =
              static_cast<double>(i) -
              static_cast<double>(next_hard_sync_reset_sample);

          const double angle_at_roll = fmod(
              current_angle_ + perc_before_roll * actual_current_angle_delta_,
              2 * juce::MathConstants<double>::twoPi);
          const double angle_before_roll = fmod(
              angle_at_roll - delta, 2 * juce::MathConstants<double>::twoPi);

          // SKEW ALL ANGLES !
          const double value_before_roll =
              GetValueAt(skew_angle(angle_before_roll));
          const double value_at_roll = GetValueAt(skew_angle(angle_at_roll));

          const double value_at_zero = GetValueAt(skew_angle(0));
          const double value_after_zero = GetValueAt(skew_angle(delta));

          // CALCULATE the MAGNITUDE of ths 1st ORDER (POS) discontinuity
          blep.pos_change_magnitude = value_at_roll - GetValueAt(skew_angle(0));

          // CALCULATE the skewed angular change AFTER the rollover
          const double angle_delta_after_roll =
              value_after_zero -
              value_at_zero;  // MODs based on the PITCH BEND ...
          const double angle_delta_before_roll =
              value_at_roll -
              value_before_roll;  // MODs based on the PITCH BEND ...

          const double change_in_delta =
              (angle_delta_after_roll - angle_delta_before_roll) *
              (1 / (2 * delta));
          const double depth_limited = blep_generator_.proportional_blep_freq_;

          // actualCurrentAngleDelta below is added to compensate for higher
          // order nonlinearities 66 here was experimentally determined ...
          blep.vel_change_magnitude = 66 * change_in_delta *
                                      (1 / depth_limited) *
                                      actual_current_angle_delta_;

          // ADD
          blep_generator_.AddBlep(blep);

          // MOVE the UNSKEWED ANGLE
          // so that it will actually roll over at this sub-sample ...
          // ESTIMATE !!!!! ERROR - this should be better, but we can't unskew
          // (x = x/cos(x) is not solvable)
          current_angle_ = 2 * juce::MathConstants<double>::twoPi -
                           perc_before_roll * actual_current_angle_delta_;

          hard_sync_blep_occurred = true;

          if (hard_sync_reset_array_idx <
              (hard_sync_reset_sample_indices_.size() - 1)) {
            next_hard_sync_reset_sample =
                hard_sync_reset_sample_indices_[hard_sync_reset_array_idx++];
          } else {
            next_hard_sync_reset_sample = -2.f;
            hard_sync_reset_array_idx = -1;
          }
        }
      }
    }

    // MOVE the ANGLE
    current_angle_ +=
        actual_current_angle_delta_;  // MODs based on the PITCH BEND ...
    current_angle_ =
        fmod(static_cast<double>(current_angle_),
             static_cast<double>(
                 2 * juce::MathConstants<double>::twoPi));  // ROLLOVER :::

    // APPLY SKEWING :::::
    // todo: what is the skewing actually used for
    current_angle_skewed_ = skew_angle(current_angle_);

    // LFO does not hard sync or anti-alias
    if constexpr (!IsLFO) {
      if (hard_sync_mode_ == PRIMARY) {
        // if we rolled over, write the subsample-accurate index of when that
        // happened
        if (current_angle_skewed_ < last_angle_skewed_) {
          // we rolled over - what's the exact sub-sample?
          // todo: probably a more efficient way to calculate this
          const auto actual_current_angle_delta_skewed =
              current_angle_skewed_ - last_angle_skewed_;
          // this will be a value between 0 and
          // actual_current_angle_delta_skewed, telling us at how many radians
          // into the sample the reset occurred todo: maybe we should stick with
          // floats here instead of doubles
          const auto reset_radians =
              actual_current_angle_delta_skewed - current_angle_skewed_;
          // scaling above value to 0 - 1 tells us exactly where in the
          // subsample the reset occurred in terms of samples rather than
          // radians
          const auto reset_subsample =
              reset_radians / actual_current_angle_delta_skewed;
          // todo: we are going to have a problem here when the reset occurs
          //  across block boundaries = i = 0 means this will actually give a
          //  negative value.
          const auto reset_sample = i - 1 + reset_subsample;
          hard_sync_reset_sample_indices_.add(static_cast<float>(reset_sample));
        }
      }

      // BUILD the antialiasing ....
      if (mode_ != NO_ANTIALIAS && hard_sync_blep_occurred == false &&
          wave_type_ != sine) {
        double actualCurrentAngleDeltaSkewed =
            current_angle_skewed_ - last_angle_skewed_;
        if (actualCurrentAngleDeltaSkewed < 0)
          actualCurrentAngleDeltaSkewed +=
              2 * juce::MathConstants<double>::twoPi;

        // ROLLED through 2*PI
        if (wave_type_ == square) {
          if (pulse_width_mod_ != 0.) {
            switch (pulse_width_mod_type_) {
              case env2Plus:
                pulse_width_actual_ =
                    static_cast<double>(env2_data[i / kOversample]) *
                    pulse_width_mod_;
                break;
              case env2Minus:
                pulse_width_actual_ =
                    static_cast<double>(env2_data[i / kOversample]) *
                    -pulse_width_mod_;
                break;
              case env1Plus:
                pulse_width_actual_ =
                    static_cast<double>(env1_data[i / kOversample]) *
                    pulse_width_mod_;
                break;
              case env1Minus:
                pulse_width_actual_ =
                    static_cast<double>(env1_data[i / kOversample]) *
                    -pulse_width_mod_;
                break;
              case lfo:
                pulse_width_actual_ =
                    (static_cast<double>(lfo_data[i / kOversample]) / 2 + 1) *
                    pulse_width_mod_;
                break;
              case manual:
                pulse_width_actual_ = pulse_width_mod_;
                break;
            }
          } else {
            pulse_width_actual_ = 0.5;
          }
          // :: SQUARE rolls twice - at pulse_width and 1 ::::
          const double threshold1 =
              juce::MathConstants<double>::twoPi * pulse_width_actual_;
          constexpr double threshold2 = 2 * juce::MathConstants<double>::twoPi;

          auto check_rollover = [&](const double threshold,
                                    const double magnitude) {
            // adjust for wrapping if needed, but current_angle_skewed_ and
            // last_angle_skewed_ should be in the same period usually unless
            // freq is very high. Actually, current_angle_skewed_ is fmodded to
            // [0, 2pi].

            bool crossed = false;
            double percAfterRoll = 0;

            if (last_angle_skewed_ < threshold &&
                current_angle_skewed_ >= threshold) {
              crossed = true;
              percAfterRoll = (current_angle_skewed_ - threshold) /
                              actualCurrentAngleDeltaSkewed;
            } else if (current_angle_skewed_ < last_angle_skewed_) {
              // Wrapped around 2PI
              if (threshold >= threshold2 - 1e-9) {
                crossed = true;
                percAfterRoll =
                    current_angle_skewed_ / actualCurrentAngleDeltaSkewed;
              } else if (last_angle_skewed_ < threshold ||
                         current_angle_skewed_ >= threshold) {
                // This case is trickier if it wraps and crosses threshold1 in
                // one sample. For now assume freq < sample_rate.
                if (last_angle_skewed_ < threshold) {
                  crossed = true;
                  percAfterRoll = (current_angle_skewed_ +
                                   (threshold2 - last_angle_skewed_) -
                                   (threshold - last_angle_skewed_)) /
                                  actualCurrentAngleDeltaSkewed;
                  // Simplify:
                  percAfterRoll =
                      (current_angle_skewed_ + threshold2 - threshold) /
                      actualCurrentAngleDeltaSkewed;
                } else if (current_angle_skewed_ >= threshold) {
                  crossed = true;
                  percAfterRoll = (current_angle_skewed_ - threshold) /
                                  actualCurrentAngleDeltaSkewed;
                }
              }
            }

            if (crossed) {
              MinBlepGenerator::BlepOffset blep;
              blep.offset = percAfterRoll - static_cast<double>(i + 1);
              blep.pos_change_magnitude = magnitude;
              blep.vel_change_magnitude = 0;
              blep_generator_.AddBlep(blep);
            }
          };

          check_rollover(threshold1, -2);
          check_rollover(threshold2, 2);
        } else if (wave_type_ == sawRise || wave_type_ == sawFall)  // SAW
        {
          // SAW ROLLs only at PI
          if (fmod(current_angle_skewed_,
                   2 * juce::MathConstants<double>::twoPi) >
                  actualCurrentAngleDeltaSkewed &&
              fmod(current_angle_skewed_, juce::MathConstants<double>::twoPi) <
                  actualCurrentAngleDeltaSkewed) {
            /*
            percAfterRoll is the fractional position (WITHING a single sample -
            a subsample) (in the current output sample) of where the waveform’s
            discontinuity (the “roll”/wrap) happened, measured as a fraction of
            one sample, but expressed as “how much of the sample occurs after
            the roll.”
            */
            double percAfterRoll =
                fmod(current_angle_skewed_,
                     juce::MathConstants<double>::twoPi) /
                actualCurrentAngleDeltaSkewed;  // LINEAR interpolation

            // CALCULATE the OFFSET
            /*
             * The offset is from the end of the output buffer.
             * It indicates where the "roll" / blep / discontinuity STARTS, at
             * an exact subsample (sample = integer part, subsample = fractional
             * part)
             */
            MinBlepGenerator::BlepOffset blep;
            blep.offset = percAfterRoll - static_cast<double>(i + 1);

            // MAGNITUDE of 1st order nonlinearity is 2 or -2 :::
            if (wave_type_ == sawRise)
              blep.pos_change_magnitude = -2;
            else
              blep.pos_change_magnitude = 2;

            // NO CHANGE to slope - 0
            blep.vel_change_magnitude = 0;

            // ADD
            blep_generator_.AddBlep(blep);
          }
        } else if (wave_type_ == triangle) {
          if (fmod(current_angle_skewed_ +
                       juce::MathConstants<double>::twoPi / 2,
                   2 * juce::MathConstants<double>::twoPi) <
                  actualCurrentAngleDeltaSkewed ||
              fmod(current_angle_skewed_ +
                       3 * juce::MathConstants<double>::twoPi / 2,
                   2 * juce::MathConstants<double>::twoPi) <
                  actualCurrentAngleDeltaSkewed) {
            double aboveNonlinearity = 0;
            double percAfterRoll = 0;

            if (fmod(current_angle_skewed_ +
                         3 * juce::MathConstants<double>::twoPi / 2,
                     2 * juce::MathConstants<double>::twoPi) <
                actualCurrentAngleDeltaSkewed) {
              aboveNonlinearity =
                  fmod(current_angle_skewed_ +
                           3 * juce::MathConstants<double>::twoPi / 2,
                       2 * juce::MathConstants<double>::twoPi);
              percAfterRoll = aboveNonlinearity / actualCurrentAngleDeltaSkewed;
            } else  // 3*double_Pi/2
            {
              aboveNonlinearity =
                  fmod(current_angle_skewed_ +
                           juce::MathConstants<double>::twoPi / 2,
                       2 * juce::MathConstants<double>::twoPi);
              percAfterRoll = aboveNonlinearity / actualCurrentAngleDeltaSkewed;
            }

            MinBlepGenerator::BlepOffset blep;
            blep.offset = percAfterRoll - static_cast<double>(i + 1);

            // SYMETRY :::::
            // since this is a triangle
            // ... we can average the two values,
            // get the abs distance from 1
            // and scale that to find the angle ....

            double nextValue = GetValueAt(current_angle_skewed_);
            double averageValue = (last_sample_ + nextValue) / 2;
            double slope = 1 - fabs(averageValue);
            jassert(slope < 1);

            double sign = 1;
            if (averageValue > 0) sign = -1;

            blep.pos_change_magnitude = 0;

            // SCALE the vel magnitude inversely with play speed
            double depthLimited =
                blep_generator_
                    .proportional_blep_freq_;  // jlimit<double>(.1, .5,
            // myBlepGenerator.proportionalBlepFreq);

            // Assume nominal delta for all waves ... so ...
            blep.vel_change_magnitude = sign * 121 * slope * (1 / depthLimited);

            // ADD
            blep_generator_.AddBlep(blep);
          }
        }
      }
    }

    *waveData = static_cast<float>(GetValueAt(current_angle_skewed_));

    // UPDATE the tracking variables ...
    // Used or computing exact values at rolls, etc.
    last_angle_skewed_ =
        current_angle_skewed_;  // NOTE the previous angle, for calculations
    last_sample_delta_ = static_cast<double>(*waveData) - last_sample_;
    last_sample_ = static_cast<double>(
        *waveData);  // NOTE* the most recent sample, for computation purposes

    waveData++;

    jassert(wave.size() >= numSamples);
  }
}

// todo: use this for PWM instead of the other stuff I used...or remove this
template <bool IsLFO>
double WaveGenerator<IsLFO>::skew_angle(const double angle) const {
  // APPLY PWM to angle ...

  // SKEW ANGLE :::::
  double skewedAngle = angle;
  if (fabs(skew_) < DELTA) {
    // ADD the INTEGRAL of SIN .. which is cos ... to the angle ...
    skewedAngle = angle + skew_ * cos(angle);
  }

  skewedAngle = fmod(skewedAngle, 2 * juce::MathConstants<double>::twoPi);
  if (skewedAngle < 0.0) skewedAngle += 2 * juce::MathConstants<double>::twoPi;

  jassert(skewedAngle >= 0 &&
          skewedAngle <= 2 * juce::MathConstants<double>::twoPi);

  return skewedAngle;
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::GetValueAt(double angle) {
  jassert(angle >= 0 && angle <= 2 * juce::MathConstants<double>::twoPi);

  double currentSample = 0;

  if (wave_type_ == sine)
    currentSample = GetSine(angle);
  else if (wave_type_ == sawRise)
    currentSample = GetSawRise(angle);
  else if (wave_type_ == sawFall)
    currentSample = GetSawFall(angle);
  else if (wave_type_ == triangle)
    currentSample = GetTriangle(angle);
  else if (wave_type_ == square)
    currentSample = GetSquare(angle, pulse_width_actual_);
  else if (wave_type_ == random)
    currentSample = GetRandom(angle);

  return currentSample;
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_bend_lfo_mod(const float mod) {
  pitch_bend_lfo_mod_ = static_cast<double>(mod);
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::set_pitch_bend_env1_mod(const float mod) {
  pitch_bend_env1_mod_ = static_cast<double>(mod);
}

// SLOW RENDER (LFO) ::::::
template <bool IsLFO>
void WaveGenerator<IsLFO>::MoveAngleForward(int numSamples) {
  // Not moving ...
  if (numSamples == 0 || (fabs(delta_base_) < DELTA)) return;

  double angle = current_angle_;
  const int historyPoints = numSamples / 20;

  // calculate MOD in the angle delta ...
  double modAngleDelta = delta_base_;

  // DISREGARDING any PITCH SHIFTING (for now, it's just not done anywhere on
  // LFOs)

  // CALCULATE the PHASE CHANGE per sample ...
  if (fabs(phase_angle_target_ - phase_angle_actual_) > DELTA) {
    // LINEAR RAMP from current phase to the target
    const float phaseShiftPerSample =
        static_cast<float>(phase_angle_target_ - phase_angle_actual_) /
        static_cast<float>(100 * numSamples);
    modAngleDelta += static_cast<double>(phaseShiftPerSample);
    phase_angle_actual_ = phase_angle_actual_ +
                          static_cast<double>(phaseShiftPerSample *
                                              static_cast<float>(numSamples));
  }

  // UPDATE the history
  for (int i = 0; i < historyPoints; i++) {
    angle = current_angle_ + i * (20 * modAngleDelta);
    angle = fmod(angle, 2 * juce::MathConstants<double>::twoPi);  // modulus
    angle = skew_angle(angle);

    history_.add(static_cast<float>(GetValueAt(angle)));

    if (history_.size() > history_length_) history_.remove(0);
  }

  current_angle_ = fmod(current_angle_ + numSamples * modAngleDelta,
                        2 * juce::MathConstants<double>::twoPi);  // ROLL
}

template <bool IsLFO>
void WaveGenerator<IsLFO>::MoveAngleForwardTo(double newAngle) {
  double delta = newAngle - current_angle_;
  if (delta < 0) delta = delta + 2 * juce::MathConstants<double>::twoPi;

  double numSamples = delta / delta_base_;

  MoveAngleForward(static_cast<int>(numSamples));
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::GetAngleAfter(
    const double samples_since_rollover) {
  // CALCULATE the WAVEFORM'S ANGULAR OFFSET
  // GIVEN a certain number of samples (since rollover) ....

  // since this is only done in LFO mode ....
  // reset phase so there is no changing ...
  phase_angle_actual_ = phase_angle_target_;

  return delta_base_ * samples_since_rollover + phase_angle_actual_;
}

template <bool IsLFO>
double WaveGenerator<IsLFO>::GetRandom([[maybe_unused]] double angle) {
  double r = static_cast<double>(juce::Random::getSystemRandom().nextFloat());

  r = 2 * (r - 0.5);  // scale to -1 .. 1
  r = juce::jlimit(-10 * delta_base_, 10 * delta_base_, r);

  last_sample_ += r;

  // limit it based on the F ...
  last_sample_ = juce::jlimit(-1.0, 1.0, last_sample_);

  return last_sample_;
}
}  // namespace audio_plugin

template class audio_plugin::WaveGenerator<true>;
template class audio_plugin::WaveGenerator<false>;