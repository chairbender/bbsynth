// Unit test for WaveGenerator sawFall rendering with BLEP
#include <BBSynth/WaveGenerator.h>
#include <gtest/gtest.h>

#include <cmath>

using audio_plugin::WaveGenerator;

namespace audio_plugin_test {
constexpr double kSampleRate = 48000.0;
constexpr double kFreq = 440.0;
constexpr int kNumSamples = 1024;

inline void PrepareAndRender(WaveGenerator& gen,
                             juce::AudioSampleBuffer& raw_buf,
                             const WaveGenerator::WaveType type) {
  gen.PrepareToPlay(kSampleRate);
  gen.set_wave_type(type);
  gen.set_pitch_hz(kFreq);
  // for more predictable results, we disable the dc blocker so there isn't
  // any filtering going on
  gen.set_dc_blocker_enabled(false);
  // build with BUILD_AA to populate BLEP offsets without consuming them
  // (so no AA filtering is going on)
  gen.set_mode(WaveGenerator::BUILD_AA);

  raw_buf.clear();
  // Warm up gain ramp so second call uses constant gain
  gen.RenderNextBlock(raw_buf, TODO, kNumSamples, TODO);
  raw_buf.clear();
  gen.blep_generator()->currentActiveBlepOffsets.clear();
  gen.RenderNextBlock(raw_buf, TODO, kNumSamples, TODO);
}

struct SawCase {
  WaveGenerator::WaveType type_;
  double expected_pos_change_; // +2 or -2
  bool ramp_up_; // true for sawFall ramp segments
  float reset_level_; // +-0.69
};

class WaveGeneratorSawTest : public ::testing::TestWithParam<SawCase> {
};

TEST_P(WaveGeneratorSawTest, RendersAndReportsBleps) {
  const auto [type, expected_pos_change, ramp_up, reset_level] = GetParam();

  WaveGenerator gen;
  juce::AudioSampleBuffer raw_buf(2, kNumSamples);
  PrepareAndRender(gen, raw_buf, type);

  // Validate BLEPs were detected at expected rate (one per period)
  auto* blep_gen = gen.blep_generator();
  const auto& bleps = blep_gen->currentActiveBlepOffsets;

  ASSERT_EQ(bleps.size(), 10);

  for (int i = 0; i < bleps.size(); ++i) {
    constexpr double kPeriodSamples = 109.09; // samplerate / freq
    const auto& b = bleps.getUnchecked(i);
    // offsets are expected to be relative to the end of the buffer they occurred in,
    // so will always be negative w.r.t. the current buffer if they occurred in the current buffer.
    // however, they will occur in reverse order in the list (closest to end of buffer is first in list)
    EXPECT_NEAR(b.offset, -12.27 - i * kPeriodSamples, 1);
    EXPECT_NEAR(b.pos_change_magnitude, expected_pos_change, 1e-3);
    // Velocity change is zero for ideal saws
    EXPECT_NEAR(b.vel_change_magnitude, 0.0, 1e-6);
  }

  const float* raw = raw_buf.getReadPointer(0);
  const float* raw2 = raw_buf.getReadPointer(1);

  // check the raw output matches the expected values
  auto prev_sample = raw[0];
  for (int i = 1; i < kNumSamples; ++i) {
    const auto sample = raw[i];
    EXPECT_NEAR(raw2[i], sample, 1e-3);
    if (ramp_up ? (sample > prev_sample) : (sample < prev_sample)) {
      // ramp segment
      EXPECT_NEAR(sample, prev_sample + (ramp_up ? +.013f : -.013f), 1e-2);
    } else {
      // reset
      EXPECT_NEAR(sample, reset_level, 1e-1);
    }
    prev_sample = sample;
  }
}

INSTANTIATE_TEST_SUITE_P(
    WaveGenerator,
    WaveGeneratorSawTest,
    ::testing::Values(
      SawCase{WaveGenerator::sawFall, +2.0, true, -.69f},
      SawCase{WaveGenerator::sawRise, -2.0, false, +.69f}
    ));


TEST(WaveGeneratorTriangleTest, RendersAndReportsTriangleBleps) {
  WaveGenerator gen;
  juce::AudioSampleBuffer raw_buf(2, kNumSamples);
  PrepareAndRender(gen, raw_buf, WaveGenerator::triangle);

  // Validate BLEPs: triangle has first-derivative discontinuities twice per period
  auto* blep_gen = gen.blep_generator();
  const auto& bleps = blep_gen->currentActiveBlepOffsets;

  EXPECT_EQ(bleps.size(), 19);

  if (bleps.size() >= 2) {
    constexpr double kPeriodSamples = kSampleRate / kFreq; // ~109.09
    constexpr double kHalfPeriod = kPeriodSamples / 2.0; // ~54.545

    // Offsets are ordered from closest to end of buffer first; spacing should be ~half-period
    for (int i = 0; i < bleps.size(); ++i) {
      const auto& b = bleps.getUnchecked(i);
      // Triangle: no position jump, but there is a velocity jump (BLAMP)
      EXPECT_NEAR(b.pos_change_magnitude, 0.0, 1e-6);
      EXPECT_NE(b.vel_change_magnitude, 0.0);

      if (i + 1 < bleps.size()) {
        const auto& next = bleps.getUnchecked(i + 1);
        // Successive BLEPs should be spaced by approximately half a period in samples
        EXPECT_NEAR(b.offset - next.offset, kHalfPeriod, 1.5);
        // Sign of velocity change should alternate at each corner
        EXPECT_LT(b.vel_change_magnitude * next.vel_change_magnitude, 0.0);
      }
    }
  }

  // Validate raw output shape: linear segments with slope magnitude ~peak_to_peak/half_period
  const float* ch0 = raw_buf.getReadPointer(0);
  const float* ch1 = raw_buf.getReadPointer(1);

  // Channels should match
  for (int i = 0; i < kNumSamples; ++i) {
    EXPECT_NEAR(ch0[i], ch1[i], 1e-3);
  }

  // Expected per-sample slope magnitude: peak-to-peak (~1.38) over half-period (~54.545)
  constexpr float kExpectedAbsSlope = 1.38f / 54.545f; // ~0.0253

  float prev = ch0[0];
  for (int i = 1; i < kNumSamples; ++i) {
    const float s = ch0[i];
    // only check when we're not crossing / near a corner
    if (std::abs((i - 3) % 55) <= 1) {
      const float diff = s - prev;
      EXPECT_NEAR(std::abs(diff), kExpectedAbsSlope, 1e-2);
    }
    prev = s;
  }
}

TEST(WaveGeneratorSquareTest, RendersAndReportsSquareBleps) {
  WaveGenerator gen;
  juce::AudioSampleBuffer raw_buf(2, kNumSamples);
  PrepareAndRender(gen, raw_buf, WaveGenerator::square);

  // BLEPs: square has position discontinuities twice per period
  auto* blep_gen = gen.blep_generator();
  const auto& bleps = blep_gen->currentActiveBlepOffsets;

  EXPECT_EQ(bleps.size(), 19);

  constexpr double kPeriodSamples = kSampleRate / kFreq; // ~109.09
  constexpr double kHalfPeriod = kPeriodSamples / 2.0; // ~54.545

  for (int i = 0; i < bleps.size(); ++i) {
    const auto& b = bleps.getUnchecked(i);
    // Square: position jump magnitude should be 2 (sign alternates), no velocity jump
    EXPECT_NEAR(std::abs(b.pos_change_magnitude), 2.0, 1e-3);
    EXPECT_NEAR(b.vel_change_magnitude, 0.0, 1e-6);

    if (i + 1 < bleps.size()) {
      const auto& next = bleps.getUnchecked(i + 1);
      // Spacing between successive BLEPs should be approximately half a period
      EXPECT_NEAR(b.offset - next.offset, kHalfPeriod, 1.5);
      // Sign of position change should alternate each transition
      EXPECT_LT(b.pos_change_magnitude * next.pos_change_magnitude, 0.0);
    }
  }

  // Raw output: two flat levels near +/-0.69 with sharp transitions; channels should match
  const float* ch0 = raw_buf.getReadPointer(0);
  const float* ch1 = raw_buf.getReadPointer(1);

  for (int i = 0; i < kNumSamples; ++i) {
    EXPECT_NEAR(ch0[i], ch1[i], 1e-3);
  }

  // Verify most adjacent samples are equal (away from transitions) and near expected levels
  float prev = ch0[0];
  for (int i = 1; i < kNumSamples; ++i) {
    const float s = ch0[i];
    const float diff = std::abs(s - prev);
    // if not a transition (keep a generous threshold), sample should stay essentially constant
    if (diff < 0.2f) {
      // level near +/-0.70
      EXPECT_NEAR(std::abs(s), .70f, 1e-2);
    }
    prev = s;
  }
}
}