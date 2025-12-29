/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/WaveGenerator.cpp
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/

// todo restore == checks against the same var as isNaN checks as that was the
// actual intention

#include "BBSynth/WaveGenerator.h"

namespace audio_plugin {

constexpr double DELTA{.0000001};

// WAVE GEN :::::
WaveGenerator::WaveGenerator(const juce::AudioBuffer<float>& lfo_buffer,
                             const juce::AudioBuffer<float>& env1_buffer,
                             const juce::AudioBuffer<float>& env2_buffer,
                             const juce::AudioBuffer<float>& modulator_buffer)
    : lfo_buffer_(lfo_buffer),
      env1_buffer_(env1_buffer),
      env2_buffer_(env2_buffer),
      modulator_buffer_{modulator_buffer} {
  history_length_ = 500;
  sample_rate_ = 0;

  mode_ = NO_ANTIALIAS;
  wave_type_ = square;

  secondary_delta_base_ = 0;
  actual_current_angle_delta_ = 0;

  current_angle_ = 0.01;  // needed
  current_angle_skewed_ = last_angle_skewed_ = 0;
  pitch_bend_target_ = pitch_bend_actual_ = 1.0;

  primary_pitch_offset_ = 1;
  secondary_pitch_offset_ = 1;

  volume_ = 1;
  gain_last_[0] = gain_last_[1] = 0;
  skew_ = 0;
  last_sample_ = 0;

  for (int i = 0; i < history_length_; i++) history_.add(0);

  phase_angle_target_ = phase_angle_actual_ = 0;  // expressed 0 - 2*PI

  // configure the blep multiple
  blep_overtone_depth_ = 128;  // high default ... buzzy
}

void WaveGenerator::PrepareToPlay(double new_sample_rate) {
  sample_rate_ = new_sample_rate;

  // BUILD the appropriate BLEP step ....
  blep_generator_.BuildBlep();
}
void WaveGenerator::set_blep_size(float newOverSample) {
  jassertfalse;  // need to rebuild, no ?

  blep_generator_.over_sampling_ratio_ = static_cast<double>(newOverSample);
}

void WaveGenerator::set_blep_overtone_depth(double mult) {
  // SET the allowed depth of overtones
  // ** NOTE **
  // this is relative to the fundamental F
  // so each factor of 2 is an octave
  blep_overtone_depth_ = mult;
}
void WaveGenerator::clear() {
  current_angle_ = phase_angle_target_;  // + phase !!!
  primary_angle_ = phase_angle_target_;

  pitch_bend_target_ = pitch_bend_actual_ = 1.0;
  secondary_delta_base_ = 0.0;
  gain_last_[0] = gain_last_[1] = 0;
}

// FAST RENDER (AP) :::::
void WaveGenerator::RenderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                    const int startSample,
                                    const int numSamples) {
  jassert(sample_rate_ != 0.);

  if (secondary_delta_base_ == 0.0) return;

  // todo FIX !!!!
  if (volume_ == 0. && gain_last_[0] == 0. && gain_last_[1] == 0. &&
      blep_generator_.IsClear())
    return;

  BuildWave(numSamples);

  // ADD BAND-LIMITED (minBLEP) transitions :::
  if (mode_ == ANTIALIAS) {
    // Since we KNOW the intended F ... relative to F(sampling)
    // We can tweak the minBLEP to limit any harmonic above 4*(desired F)

    // TUNE the blep ....
    double freq = current_pitch_hz();               // Current, playing, Freq
    double relativeFreq = 2 * freq / sample_rate_;  // 2 for Nyquist ...
    relativeFreq *= blep_overtone_depth_;  // ie - up to the 3nd harmonic (2*2*2
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
      auto samples = wave.getRawDataPointer();
      const float R = 0.995f;  // pole close to 1.0

      // Preserve last raw input of this block (before we overwrite samples)
      const float lastInputRaw = samples[numSamples - 1];

      // Use persistent states from previous block
      float xPrev = static_cast<float>(prev_buffer_last_sample_raw_);
      float yPrev = static_cast<float>(prev_buffer_last_sample_filtered_);

      for (int i = 0; i < numSamples; ++i) {
        const float x = samples[i];
        const float y = (x - xPrev) + R * yPrev;
        samples[i] = y;
        xPrev = x;
        yPrev = y;
      }

      // Update states for next block
      prev_buffer_last_sample_raw_ = static_cast<double>(lastInputRaw);
      prev_buffer_last_sample_filtered_ = static_cast<double>(yPrev);
    }
  }

  // BUILD ::::
  for (int i = 0; i < numSamples; i = i + 20) {
    // just adding a sample every 20 or so to the history
    history_.add(wave.getUnchecked(i));

    if (history_.size() > history_length_) history_.remove(0);
  }

  // COPY it to the outputbuffer ....
  outputBuffer.addFromWithRamp(0, startSample, wave.getRawDataPointer(),
                               numSamples, static_cast<float>(gain_last_[0]),
                               static_cast<float>(volume_));

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
inline void WaveGenerator::BuildWave(const int numSamples) {
  if (secondary_delta_base_ == 0.0) return;

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
  const auto lfo_data = lfo_buffer_.getReadPointer(0);
  const auto env1_data = env1_buffer_.getReadPointer(0);
  const auto env2_data = env2_buffer_.getReadPointer(0);
  const auto modulator_data = modulator_buffer_.getReadPointer(0);
  for (int i = 0; i < numSamples; i++) {
    bool primary_blep_occurred = false;

    // CHANGE the PITCH BEND (linear ramping)
    // TODO: manual pitch bend disabled currently
    // pitch_bend_actual_ += freqDelta;
    // TODO: account better for oversampling - this hardcoded amount isn't good
    double mod = 0;
    if (pitch_bend_lfo_mod_ != 0.) {
      mod = static_cast<double>(lfo_data[i / 2]) * pitch_bend_lfo_mod_;
    }
    if (pitch_bend_env1_mod_ != 0.) {
      mod += static_cast<double>(env1_data[i / 2]) * pitch_bend_env1_mod_;
    }
    if (cross_mod_ != 0.) {
      // unlike the other modulation buffers, the modulator is oversampled.
      // note blepping has already been applied to the modulator signal
      // so the carrier only needs to deal with its own discontinuities like
      // normal todo: is this reasoning actually correct for what blepping is
      // needed?
      // todo: this is not producing the expected sound...
      mod += static_cast<double>(modulator_data[i]) * cross_mod_;
    }
    if (mod != 0.) {
      pitch_bend_actual_ = 1 + mod;
    }

    if (fabs(pitch_bend_actual_ - 1) < .00001) pitch_bend_actual_ = 1;

    // FOR CALCULATIONS,
    // note the current, actual delta (base pitch modified by pitch bends and
    // phase shifting)
    actual_current_angle_delta_ =
        secondary_delta_base_ * pitch_bend_actual_ + phaseShiftPerSample;

    // HARD SYNCING ::::::
    // TODO: seemingly the pitch offset is being ignored completely unless hard
    // sync is true...
    if (hard_sync_ &&
        (fabs(primary_delta_base_ - secondary_delta_base_) > DELTA)) {
      // primary OSC DOES use pitch bending and phase shifting ...
      const double actual_current_primary_delta =
          primary_delta_base_ * pitch_bend_actual_ + phaseShiftPerSample;
      primary_angle_ += actual_current_primary_delta;

      // ADD A BLEP ::::
      primary_angle_ =
          fmod(primary_angle_,
               static_cast<double>(2 * juce::MathConstants<double>::twoPi));

      // primary (unskewed) rollover
      if (primary_angle_ < actual_current_primary_delta) {
        double percAfterRoll = primary_angle_ / actual_current_primary_delta;
        double percBeforeRoll = 1 - percAfterRoll;

        // ADD the blep ...
        {
          MinBlepGenerator::BlepOffset blep;
          blep.offset = percAfterRoll - static_cast<double>(i + 1);

          // CALCULATE the MAGNITUDE of ths 2nd ORDER (VEL) discontinuity
          // TRIG :: calculate the angle (rise/run) before and after the
          // rollover
          double delta = .0000001;  // MIN

          double angleAtRoll = fmod(
              current_angle_ + percBeforeRoll * actual_current_angle_delta_,
              2 * juce::MathConstants<double>::twoPi);
          double angleBeforeRoll =
              fmod(angleAtRoll - delta, 2 * juce::MathConstants<double>::twoPi);

          // SKEW ALL ANGLES !
          double valueBeforeRoll = GetValueAt(skew_angle(angleBeforeRoll));
          double valueAtRoll = GetValueAt(skew_angle(angleAtRoll));

          double valueAtZero = GetValueAt(skew_angle(0));
          double valueAfterZero = GetValueAt(skew_angle(delta));

          // CALCULATE the MAGNITUDE of ths 1st ORDER (POS) discontinuity
          blep.pos_change_magnitude = valueAtRoll - GetValueAt(skew_angle(0));

          // CALCULATE the skewed angular change AFTER the rollover
          double angle_delta_after_roll =
              valueAfterZero - valueAtZero;  // MODs based on the PITCH BEND ...
          double angle_delta_before_roll =
              valueAtRoll -
              valueBeforeRoll;  // MODs based on the PITCH BEND ...

          double change_in_delta =
              (angle_delta_after_roll - angle_delta_before_roll) *
              (1 / (2 * delta));
          double depthLimited =
              blep_generator_
                  .proportional_blep_freq_;  // jlimit<double>(.1, .5,
                                             // myBlepGenerator.proportionalBlepFreq);

          // actualCurrentAngleDelta below is added to compensate for higher
          // order nonlinearities 66 here was experimentally determined ...
          blep.vel_change_magnitude = 66 * change_in_delta *
                                      (1 / depthLimited) *
                                      actual_current_angle_delta_;

          // ADD
          blep_generator_.AddBlep(blep);
        }

        // MOVE the UNSKEWED ANGLE
        // so that it will actually roll over at this sub-sample ...
        // ESTIMATE !!!!! ERROR - this should be better, but we can't unskew (x
        // = x/cos(x) is not solvable)
        current_angle_ = 2 * juce::MathConstants<double>::twoPi -
                         percBeforeRoll * actual_current_angle_delta_;

        //
        primary_blep_occurred = true;
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
    current_angle_skewed_ = skew_angle(current_angle_);

    // BUILD the antialiasing ....
    if (mode_ != NO_ANTIALIAS && primary_blep_occurred == false &&
        wave_type_ != sine) {
      double actualCurrentAngleDeltaSkewed =
          current_angle_skewed_ - last_angle_skewed_;
      if (actualCurrentAngleDeltaSkewed < 0)
        actualCurrentAngleDeltaSkewed += 2 * juce::MathConstants<double>::twoPi;

      // ROLLED through 2*PI
      if (wave_type_ == square) {
        if (pulse_width_mod_ != 0.) {
          switch (pulse_width_mod_type_) {
            case env2Plus:
              pulse_width_actual_ =
                  static_cast<double>(env2_data[i / 2]) * pulse_width_mod_;
              break;
            case env2Minus:
              pulse_width_actual_ =
                  static_cast<double>(env2_data[i / 2]) * -pulse_width_mod_;
              break;
            case env1Plus:
              pulse_width_actual_ =
                  static_cast<double>(env1_data[i / 2]) * pulse_width_mod_;
              break;
            case env1Minus:
              pulse_width_actual_ =
                  static_cast<double>(env1_data[i / 2]) * -pulse_width_mod_;
              break;
            case lfo:
              pulse_width_actual_ =
                  (static_cast<double>(lfo_data[i / 2]) / 2 + 1) *
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
          // last_angle_skewed_ should be in the same period usually unless freq
          // is very high. Actually, current_angle_skewed_ is fmodded to [0,
          // 2pi].

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
              // This case is trickier if it wraps and crosses threshold1 in one
              // sample. For now assume freq < sample_rate.
              if (last_angle_skewed_ < threshold) {
                crossed = true;
                percAfterRoll =
                    (current_angle_skewed_ + (threshold2 - last_angle_skewed_) -
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
          percAfterRoll is the fractional position (WITHING a single sample - a
          subsample) (in the current output sample) of where the waveform’s
          discontinuity (the “roll”/wrap) happened, measured as a fraction of
          one sample, but expressed as “how much of the sample occurs after the
          roll.”
          */
          double percAfterRoll =
              fmod(current_angle_skewed_, juce::MathConstants<double>::twoPi) /
              actualCurrentAngleDeltaSkewed;  // LINEAR interpolation

          // CALCULATE the OFFSET
          /*
           * The offset is from the end of the output buffer.
           * It indicates where the "roll" / blep / discontinuity STARTS, at an
           * exact subsample (sample = integer part, subsample = fractional
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
        if (fmod(current_angle_skewed_ + juce::MathConstants<double>::twoPi / 2,
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
            aboveNonlinearity = fmod(
                current_angle_skewed_ + juce::MathConstants<double>::twoPi / 2,
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

    *waveData = static_cast<float>(GetValueAt(current_angle_skewed_));

    // UPDATE the tracking variables ...
    // Used or computing exact values at rolls, etc.
    last_angle_skewed_ =
        current_angle_skewed_;  // NOTE the previous angle, for calculations
    last_sample_delta_ = static_cast<double>(*waveData) - last_sample_;
    last_sample_ = static_cast<double>(
        *waveData);  // NOTE* the most recent sample, for computation purposes

    waveData++;
  }

  jassert(wave.size() >= numSamples);
}

double WaveGenerator::skew_angle(const double angle) const {
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
double WaveGenerator::GetValueAt(double angle) {
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

void WaveGenerator::set_pitch_bend_lfo_mod(const float mod) {
  pitch_bend_lfo_mod_ = static_cast<double>(mod);
}

void WaveGenerator::set_pitch_bend_env1_mod(const float mod) {
  pitch_bend_env1_mod_ = static_cast<double>(mod);
}

// SLOW RENDER (LFO) ::::::
void WaveGenerator::MoveAngleForward(int numSamples) {
  // Not moving ...
  if (numSamples == 0 || (fabs(secondary_delta_base_) < DELTA)) return;

  double angle = current_angle_;
  const int historyPoints = numSamples / 20;

  // calculate MOD in the angle delta ...
  double modAngleDelta = secondary_delta_base_;

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
void WaveGenerator::MoveAngleForwardTo(double newAngle) {
  double delta = newAngle - current_angle_;
  if (delta < 0) delta = delta + 2 * juce::MathConstants<double>::twoPi;

  double numSamples = delta / secondary_delta_base_;

  MoveAngleForward(static_cast<int>(numSamples));
}

// LOAD / SAVE
void WaveGenerator::LoadScene(const juce::ValueTree& node) {
  // wavetype ....
  int type = node.getProperty("waveType", 0);
  // todo is this the right way to cast an enum?
  set_wave_type(static_cast<WaveType>(type));

  primary_pitch_offset_ = node.getProperty("primaryPitchOffset", 1.0);

  volume_ = node.getProperty("volume", volume_);
  skew_ = node.getProperty("skew", skew_);

  blep_overtone_depth_ = node.getProperty("blepOvertoneDepth", 64.);
  phase_angle_target_ =
      node.getProperty("phaseAngleTarget", phase_angle_target_);

  // ALWAYS HARD SYNC (for now)
  hard_sync_ = true;  //(bool)node.getProperty("hardSync", false);

  // TONE :::
  float tone = node.getProperty("toneOffset", 0);
  set_tone_offset(static_cast<double>(tone));
}

void WaveGenerator::SaveScene(juce::ValueTree node) const {
  node.setProperty("waveType", wave_type_, nullptr);
  node.setProperty("primaryPitchOffset", primary_pitch_offset_, nullptr);

  node.setProperty("volume", volume_, nullptr);
  node.setProperty("skew", skew_, nullptr);

  node.setProperty("blepOvertoneDepth", blep_overtone_depth_, nullptr);
  node.setProperty("phaseAngleTarget", phase_angle_target_, nullptr);

  node.setProperty("hardSync", hard_sync_, nullptr);  // always true!

  // TONE :::
  node.setProperty("toneOffset", tone_offset_in_semis(), nullptr);
}

}  // namespace audio_plugin