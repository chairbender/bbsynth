// Unit test for WaveGenerator sawFall rendering with BLEP
#include <BBSynth/WaveGenerator.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <vector>

using audio_plugin::WaveGenerator;

namespace audio_plugin_test {

// static double median(std::vector<double>& v) {
//   if (v.empty()) return 0.0;
//   std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
//   return v[v.size() / 2];
// }

// Count how many very large discontinuities (wraps) are present by looking for
// diffs far from the typical per-sample delta of the ramp.
static int countLargeJumps(const float* data, int n) {
  std::vector<double> diffs;
  diffs.reserve(static_cast<size_t>(n - 1));
  for (int i = 1; i < n; ++i) diffs.push_back(static_cast<double>(data[i] - data[i - 1]));

  // Estimate a typical delta magnitude via the median
  std::vector<double> mags(diffs.size());
  std::transform(diffs.begin(), diffs.end(), mags.begin(), [](double d) { return std::abs(d); });
  double medMag = 0.0;
  if (!mags.empty()) {
    std::nth_element(mags.begin(), mags.begin() + mags.size() / 2, mags.end());
    medMag = mags[mags.size() / 2];
  }

  if (medMag <= 0.0) medMag = 1e-9;  // avoid div by zero

  int jumps = 0;
  for (double d : diffs) {
    if (std::abs(d) > 8.0 * medMag) // a wrap is a much larger change than slope steps
      ++jumps;
  }
  return jumps;
}

TEST(WaveGenerator, RendersSawFallAndReportsBleps) {
  constexpr double sampleRate = 48000.0;
  constexpr double freq = 440.0;
  constexpr int numSamples = 1024;

  WaveGenerator gen;
  gen.prepareToPlay(sampleRate);
  gen.setWaveType(WaveGenerator::sawFall);
  gen.setPitchHz(freq);
  // for more predictable results, we disable the dc blocker so there isn't
  // any filtering going on
  gen.setDcBlockerEnabled(false);

  // First, build with BUILD_AA to populate BLEP offsets without consuming them
  // (so no AA filtering is going on)
  gen.setMode(WaveGenerator::BUILD_AA);

  juce::AudioSampleBuffer rawBuf(2, numSamples);
  rawBuf.clear();
  // Warm up gain ramp so second call uses constant gain
  gen.renderNextBlock(rawBuf, numSamples);
  rawBuf.clear();
  gen.getBlepGenerator()->currentActiveBlepOffsets.clear();
  gen.renderNextBlock(rawBuf, numSamples);

  // Validate BLEPs were detected at expected approximate rate (one per period)
  auto* blepGen = gen.getBlepGenerator();
  const auto& bleps = blepGen->currentActiveBlepOffsets;

  // Expected number of wraps in this block
  const double periodSamples = sampleRate / freq; // ~109.09
  const int expectedWraps = static_cast<int>(std::floor(numSamples / periodSamples));

  ASSERT_EQ(bleps.size(), 10);

  // For sawFall, we expect a position discontinuity of positive magnitude (code sets +2)
  // TODO left off here - improving the expectations to be more precise
  for (int i = 0; i < bleps.size(); ++i) {
    const auto& b = bleps.getUnchecked(i);
    EXPECT_LE(b.offset, 0.0) << "BLEP offset should be <= 0 within the current buffer (BUILD_AA).";
    EXPECT_NEAR(b.pos_change_magnitude, 2.0, 1e-3) << "sawFall should report +2 position change magnitude.";
    // Velocity change is zero for ideal saws
    EXPECT_NEAR(b.vel_change_magnitude, 0.0, 1e-6);
  }

  // The raw (no BLEP applied) saw should show large discontinuities once per period
  const float* raw = rawBuf.getReadPointer(0);
  const int rawJumps = countLargeJumps(raw, numSamples);
  EXPECT_GE(rawJumps, std::max(0, expectedWraps - 1));
  EXPECT_LE(rawJumps, expectedWraps + 1);

  // Between jumps, the ramp should be generally increasing (sawFall implementation rises)
  // Compute average diff excluding the detected jump outliers
  std::vector<double> diffs;
  diffs.reserve(static_cast<size_t>(numSamples - 1));
  for (int i = 1; i < numSamples; ++i) diffs.push_back(static_cast<double>(raw[i] - raw[i - 1]));
  // Remove very large negative outliers (wraps)
  std::vector<double> mags(diffs.size());
  std::transform(diffs.begin(), diffs.end(), mags.begin(), [](double d) { return std::abs(d); });
  double medMag = 0.0;
  if (!mags.empty()) {
    std::nth_element(mags.begin(), mags.begin() + mags.size() / 2, mags.end());
    medMag = mags[mags.size() / 2];
  }
  double sumSmall = 0.0;
  int countSmall = 0;
  for (double d : diffs) {
    if (std::abs(d) <= 8.0 * medMag) {
      sumSmall += d;
      ++countSmall;
    }
  }
  ASSERT_GT(countSmall, 0);
  EXPECT_GT(sumSmall / countSmall, 0.0);
}

}  // namespace audio_plugin_test
