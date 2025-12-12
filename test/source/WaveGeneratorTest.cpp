// Unit test for WaveGenerator sawFall rendering with BLEP
#include <BBSynth/WaveGenerator.h>
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

using audio_plugin::WaveGenerator;

namespace audio_plugin_test {

struct SawCase {
  WaveGenerator::WaveType type_;
  double expected_pos_change_;   // +2 or -2
  bool ramp_up_;                 // true for sawFall ramp segments
  float reset_level_;            // +-0.69
};

class WaveGeneratorSawTest : public ::testing::TestWithParam<SawCase> {};

TEST_P(WaveGeneratorSawTest, RendersAndReportsBleps) {
  constexpr double kSampleRate = 48000.0;
  constexpr double kFreq = 440.0;
  constexpr int kNumSamples = 1024;

  const auto [type, expected_pos_change, ramp_up, reset_level] = GetParam();

  WaveGenerator gen;
  gen.prepareToPlay(kSampleRate);
  gen.setWaveType(type);
  gen.setPitchHz(kFreq);
  // for more predictable results, we disable the dc blocker so there isn't
  // any filtering going on
  gen.setDcBlockerEnabled(false);

  // build with BUILD_AA to populate BLEP offsets without consuming them
  // (so no AA filtering is going on)
  gen.setMode(WaveGenerator::BUILD_AA);

  juce::AudioSampleBuffer raw_buf(2, kNumSamples);
  raw_buf.clear();
  // Warm up gain ramp so second call uses constant gain
  gen.renderNextBlock(raw_buf, kNumSamples);
  raw_buf.clear();
  gen.getBlepGenerator()->currentActiveBlepOffsets.clear();
  gen.renderNextBlock(raw_buf, kNumSamples);

  // Validate BLEPs were detected at expected rate (one per period)
  auto* blep_gen = gen.getBlepGenerator();
  const auto& bleps = blep_gen->currentActiveBlepOffsets;

  ASSERT_EQ(bleps.size(), 10);

  for (int i = 0; i < bleps.size(); ++i) {
    constexpr double kPeriodSamples = 109.09; // samplerate / freq
    const auto& b = bleps.getUnchecked(i);
    // offsets are expected to be relative to the end of the buffer they occurred in,
    // so will always be negative w.r.t. the current buffer if they occurred in the current buffer.
    // however, they will occur in reverse order in the list (closest to end of buffer is first in list)
    EXPECT_NEAR(b.offset, -12.27 - i * kPeriodSamples , 1);
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


}  // namespace audio_plugin_test
