// Unit test for WaveGenerator sawFall rendering with BLEP
#include <BBSynth/WaveGenerator.h>
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

using audio_plugin::WaveGenerator;

namespace audio_plugin_test {

struct SawCase {
  WaveGenerator::WaveType type;
  double expected_pos_change;   // +2 or -2
  bool ramp_up;                 // true for sawFall ramp segments
  float reset_level;            // +-0.69
};

class WaveGeneratorSawTest : public ::testing::TestWithParam<SawCase> {};

TEST_P(WaveGeneratorSawTest, RendersAndReportsBleps) {
  constexpr double sampleRate = 48000.0;
  constexpr double freq = 440.0;
  constexpr int numSamples = 1024;

  const auto [type, expectedPosChange, rampUp, resetLevel] = GetParam();

  WaveGenerator gen;
  gen.prepareToPlay(sampleRate);
  gen.setWaveType(type);
  gen.setPitchHz(freq);
  // for more predictable results, we disable the dc blocker so there isn't
  // any filtering going on
  gen.setDcBlockerEnabled(false);

  // build with BUILD_AA to populate BLEP offsets without consuming them
  // (so no AA filtering is going on)
  gen.setMode(WaveGenerator::BUILD_AA);

  juce::AudioSampleBuffer rawBuf(2, numSamples);
  rawBuf.clear();
  // Warm up gain ramp so second call uses constant gain
  gen.renderNextBlock(rawBuf, numSamples);
  rawBuf.clear();
  gen.getBlepGenerator()->currentActiveBlepOffsets.clear();
  gen.renderNextBlock(rawBuf, numSamples);

  // Validate BLEPs were detected at expected rate (one per period)
  auto* blepGen = gen.getBlepGenerator();
  const auto& bleps = blepGen->currentActiveBlepOffsets;

  ASSERT_EQ(bleps.size(), 10);

  for (int i = 0; i < bleps.size(); ++i) {
    constexpr double periodSamples = 109.09; // samplerate / freq
    const auto& b = bleps.getUnchecked(i);
    // offsets are expected to be relative to the end of the buffer they occurred in,
    // so will always be negative w.r.t. the current buffer if they occurred in the current buffer.
    // however, they will occur in reverse order in the list (closest to end of buffer is first in list)
    EXPECT_NEAR(b.offset, -12.27 - i * periodSamples , 1);
    EXPECT_NEAR(b.pos_change_magnitude, expectedPosChange, 1e-3);
    // Velocity change is zero for ideal saws
    EXPECT_NEAR(b.vel_change_magnitude, 0.0, 1e-6);
  }

  const float* raw = rawBuf.getReadPointer(0);
  const float* raw2 = rawBuf.getReadPointer(1);

  // check the raw output matches the expected values
  auto prevSample = raw[0];
  for (int i = 1; i < numSamples; ++i) {
    const auto sample = raw[i];
    EXPECT_NEAR(raw2[i], sample, 1e-3);
    if (rampUp ? (sample > prevSample) : (sample < prevSample)) {
      // ramp segment
      EXPECT_NEAR(sample, prevSample + (rampUp ? +.013f : -.013f), 1e-2);
    } else {
      // reset
      EXPECT_NEAR(sample, resetLevel, 1e-1);
    }
    prevSample = sample;
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
