// Unit test for MinBlepGenerator
#include <../../plugin/source/oscillator/MinBlepGenerator.h>
#include <gtest/gtest.h>

#include <cmath>

namespace audio_plugin_test {
// expected values for the blep table - this
//  helps ensure we don't accidentally change the table calculation
constexpr auto kExpectedMinBlepTable = std::to_array({
    0.999986, 0.99999, 1.00001, 1.00004, 1.00008, 1.00013, 1.00017, 1.0002,
    1.00022, 1.00019, 1.00013, 0.999993, 0.999775, 0.999447, 0.998979, 0.998337,
    0.99748, 0.99636, 0.994922, 0.993102, 0.990828, 0.988018, 0.984581,
    0.980417, 0.975414, 0.969453, 0.962406, 0.95414, 0.944513, 0.933381, 0.9206,
    0.906023, 0.889513, 0.870935, 0.85017, 0.827111, 0.80167, 0.773784,
    0.743416, 0.71056, 0.675244, 0.637533, 0.597533, 0.555393, 0.511303,
    0.465497, 0.418255, 0.369894, 0.320774, 0.271288, 0.22186, 0.172938,
    0.124989, 0.0784887, 0.0339159, -0.00825846, -0.0475799, -0.0836201,
    -0.115986, -0.144331, -0.168361, -0.187843, -0.202614, -0.212586, -0.217747,
    -0.218169, -0.214005, -0.20549, -0.192937, -0.176733, -0.15733, -0.13524,
    -0.111025, -0.0852795, -0.0586252, -0.0316936, -0.00511193, 0.0205097,
    0.0445933, 0.0666068, 0.0860753, 0.102594, 0.115836, 0.125563, 0.131626,
    0.133973, 0.132648, 0.127789, 0.119624, 0.108463, 0.0946921, 0.0787582,
    0.0611597, 0.0424305, 0.0231256, 0.00380552, -0.0149803, -0.0327083,
    -0.0488952, -0.063112, -0.0749952, -0.0842564, -0.0906904, -0.094179,
    -0.0946947, -0.0922989, -0.0871391, -0.0794437, -0.0695132, -0.0577103,
    -0.0444473, -0.0301734, -0.0153593, -0.000482678, 0.013987, 0.027603,
    0.0399551, 0.0506822, 0.0594827, 0.0661231, 0.0704443, 0.0723658, 0.0718863,
    0.0690824, 0.0641051, 0.0571729, 0.0485637, 0.0386046, 0.0276595, 0.0161172,
    0.00437742, -0.00716293, -0.0181223, -0.0281478, -0.0369257, -0.0441922,
    -0.0497402, -0.0534263, -0.0551734, -0.0549729, -0.0528823, -0.0490232,
    -0.0435743, -0.0367656, -0.0288677, -0.0201827, -0.0110326, -0.00174737,
    0.00734609, 0.0159362, 0.0237364, 0.0304951, 0.0360036, 0.0401027,
    0.0426867, 0.0437062, 0.0431684, 0.0411356, 0.0377215, 0.033086, 0.0274282,
    0.0209784, 0.0139892, 0.00672561, -0.000544429, -0.00756037, -0.0140777,
    -0.0198764, -0.0247688, -0.0286051, -0.0312779, -0.0327247, -0.0329298,
    -0.0319227, -0.0297763, -0.0266029, -0.0225492, -0.0177896, -0.0125195,
    -0.00694692, -0.00128508, 0.00425571, 0.00947624, 0.0141949, 0.0182541,
    0.0215253, 0.0239133, 0.0253583, 0.0258372, 0.0253639, 0.0239866, 0.0217862,
    0.0188712, 0.0153736, 0.0114428, 0.00723958, 0.00292951, -0.00132298,
    -0.00536144, -0.00904191, -0.0122393, -0.0148509, -0.0167987, -0.0180333,
    -0.0185333, -0.0183059, -0.0173856, -0.0158318, -0.0137259, -0.0111672,
    -0.00826895, -0.00515306, -0.00194538, 0.00122923, 0.00425172, 0.00701302,
    0.00941819, 0.0113893, 0.0128678, 0.0138164, 0.0142193, 0.0140821,
    0.0134313, 0.0123118, 0.0107848, 0.00892484, 0.00681663, 0.00455081,
    0.00222075, -0.0000815392, -0.00226855, -0.00426078, -0.00598979,
    -0.00739944, -0.00844896, -0.00911272, -0.00938129, -0.00926101,
    -0.00877321, -0.00795233, -0.00684476, -0.00550568, -0.00399733,
    -0.00238574, -0.000738144, 0.000879586, 0.00240576, 0.00378478, 0.00496948,
    0.00592244, 0.00661659, 0.00703681, 0.0071792, 0.00705081, 0.00666934,
    0.00606173, 0.0052622, 0.00431162, 0.00325441, 0.00213736, 0.0010075,
    -0.0000898838, -0.00111306, -0.00202501, -0.00279522, -0.00340021,
    -0.00382411, -0.00405931, -0.00410616, -0.00397253, -0.0036732, -0.00322926,
    -0.00266647, -0.00201428, -0.00130451, -0.000570059, 0.000156701,
    0.000844955, 0.00146657, 0.00199711, 0.00241685, 0.00271076, 0.00286931,
    0.00288838, 0.00276911, 0.0025177, 0.00214452, 0.00166404, 0.00109351,
    0.000452697, -0.000237584, -0.00095582, -0.00168097, -0.00239289,
    -0.00307333, -0.00370574, -0.00427687, -0.00477576, -0.00519466,
    -0.00552845, -0.00577509, -0.00593472, -0.00600994, -0.00600517,
    -0.00592566, -0.00577819, -0.00556982, -0.00530767, -0.00499856,
    -0.00464928, -0.00426555, -0.00385237, -0.00341415, -0.0029546, -0.00247669,
    -0.00198317, -0.00147629, -0.0009588, -0.000433445, 0.0000964999,
    0.000626862, 0.00115275, 0.00166833, 0.00216663, 0.00263995, 0.00307983,
    0.00347728, 0.00382292, 0.00410765, 0.00432259, 0.00446004, 0.00451326,
    0.00447744, 0.00434947, 0.00412899, 0.0038178, 0.00342065, 0.00294495,
    0.0024007, 0.0018006, 0.00115943, 0.000493765, -0.000178456, -0.000838041,
    -0.00146604, -0.00204372, -0.00255346, -0.00297916, -0.0033071, -0.00352633,
    -0.00362885, -0.00361085, -0.00347161, -0.00321496, -0.00284803, -0.0023818,
    -0.00183058, -0.00121176, -0.000545025, 0.000148535, 0.000846386,
    0.00152606, 0.00216532, 0.0027436, 0.00324184, 0.00364417, 0.00393748,
    0.00411248, 0.00416422, 0.0040915, 0.00389761, 0.00358987, 0.00317961,
    0.00268149, 0.0021131, 0.00149453, 0.000847399, 0.000194132, -0.000442743,
    -0.00104153, -0.00158179, -0.00204539, -0.00241745, -0.00268579,
    -0.00284255, -0.00288343, -0.00280881, -0.00262272, -0.00233316,
    -0.00195193, -0.00149369, -0.000976086, -0.000418186, 0.000159383,
    0.00073576, 0.00129044, 0.00180376, 0.00225794, 0.00263768, 0.0029307,
    0.00312763, 0.0032233, 0.00321579, 0.00310743, 0.00290388, 0.00261432,
    0.00225103, 0.00182849, 0.00136358, 0.000873864, 0.000378132, -0.0001055,
    -0.00055933, -0.000967264, -0.00131512, -0.00159121, -0.00178707,
    -0.00189674, -0.00191832, -0.00185287, -0.00170469, -0.00148106,
    -0.00119221, -0.000850439, -0.000469804, -0.0000653267, 0.000347137,
    0.000751734, 0.00113344, 0.00147861, 0.00177473, 0.00201207, 0.0021829,
    0.0022825, 0.00230879, 0.00226277, 0.00214803, 0.00197059, 0.00173903,
    0.00146359, 0.00115597, 0.000828862, 0.000495374, 0.000168443, -0.000139475,
    -0.000417113, -0.000654578, -0.000843763, -0.000978708, -0.00105596,
    -0.00107408, -0.00103402, -0.000939012, -0.000794649, -0.000607848,
    -0.000387192, -0.000142336, 0.000116348, 0.00037843, 0.000633538,
    0.000871837, 0.00108451, 0.00126398, 0.00140435, 0.00150138, 0.00155252,
    0.00155747, 0.00151736, 0.0014354, 0.00131625, 0.00116593, 0.000991285,
    0.000800312, 0.000600934, 0.000401556, 0.000210166, 0.0000338554,
    -0.00012064, -0.000248075, -0.000344276, -0.000406623, -0.000433683,
    -0.000425935, -0.000384688, -0.000313044, -0.000215054, -0.0000956059,
    0.0000396371, 0.000184655, 0.00033313, 0.000479043, 0.00061655, 0.000740409,
    0.000846028, 0.000930011, 0.000989735, 0.00102389, 0.00103194, 0.00101477,
    0.000974119, 0.000912607, 0.000833571, 0.000741065, 0.000639319,
    0.000533164, 0.000426829, 0.000324965, 0.000231385, 0.000149667,
    0.0000824928, 0.0000321865, 0.0000000596046});

TEST(MinBlepGenerator, BuildBlep_MatchesExpectedTable) {
  // Arrange
  audio_plugin::MinBlepGenerator gen;

  // Act: explicitly build the BLEP table
  gen.BuildBlep();

  // assert table within expected values
  const auto arr = audio_plugin::MinBlepGenerator::min_blep_array();
  ASSERT_EQ(arr.size(), 512);
  for (unsigned long i = 0; i < 512; i++) {
    EXPECT_NEAR(arr[static_cast<int>(i)], kExpectedMinBlepTable[i], 1.0e-3f);
  }
}

TEST(MinBlepGenerator, ProcessBlock_Applies0ThOrderBlep) {
  audio_plugin::MinBlepGenerator gen;

  constexpr int kNumSamples = 64;
  float buffer[kNumSamples] = {};

  // Create a BLEP that occurs within the current buffer (negative offset)
  // and only has a 0th-order (position) discontinuity.
  audio_plugin::MinBlepGenerator::BlepOffset blep;
  blep.offset = -32.0; // happened about halfway through the current buffer
  blep.freqMultiple = 8.0;
  // sensible value: 16x oversample * 0.5 proportional freq = 8
  blep.pos_change_magnitude = 2.0; // apply a noticeable correction
  blep.vel_change_magnitude = 0.0; // ensure we're only testing 0th-order BLEP

  // Add directly to the active BLEPs for this block
  gen.currentActiveBlepOffsets.add(blep);

  const auto blepTable = audio_plugin::MinBlepGenerator::min_blep_array();
  const float expectedFirst = static_cast<float>(blep.pos_change_magnitude) *
                              blepTable.getUnchecked(0);

  gen.ProcessBlock(buffer, kNumSamples);

  // all samples should stay 0 (unmodified) until the blep
  for (int i = 0; i < 31; ++i) {
    ASSERT_EQ(buffer[i], 0.0f);
  }

  // Assert: the BLEP should be applied to the buffer
  // Check the very first sample matches the expected scaled BLEP start value
  EXPECT_NEAR(buffer[31], expectedFirst, 1.0e-5f);

  // blep should be added to the signal for the remaining samples
  for (int i = 32; i < kNumSamples; ++i) {
    // Mirror the production stepping logic:
    // sampleExact = freqMultiple * (offset + i + 1)
    const double outputSamplesSinceBlep =
        static_cast<double>(blep.offset) + static_cast<double>(i) + 1.0;
    const double sampleExact = static_cast<double>(blep.freqMultiple) *
                               outputSamplesSinceBlep;

    float expected = 0.0f;
    double sampleIndexDouble = 0.0;
    const double frac = std::modf(sampleExact, &sampleIndexDouble);
    const int sampleIndex = static_cast<int>(sampleIndexDouble);

    const float before = blepTable.getUnchecked(sampleIndex);
    const float after = (sampleIndex + 1 < blepTable.size())
                          ? blepTable.getUnchecked(sampleIndex + 1)
                          : before;
    const float interp = before + static_cast<float>(frac) * (after - before);
    expected = static_cast<float>(blep.pos_change_magnitude) * interp;

    EXPECT_NEAR(buffer[i], expected, 1.0e-5f);
  }
}

TEST(MinBlepGenerator, ProcessBlock_Applies1StOrderBlep) {
  audio_plugin::MinBlepGenerator gen;

  constexpr int kNumSamples = 64;
  float buffer[kNumSamples] = {};

  // Create a BLEP that occurs within the current buffer (negative offset)
  // and only has a 1st-order (velocity) discontinuity.
  audio_plugin::MinBlepGenerator::BlepOffset blep;
  blep.offset = -32.0; // happened about halfway through the current buffer
  blep.freqMultiple = 8.0; // used for initial gating in process_currentBleps
  blep.pos_change_magnitude = 0.0; // ensure we're only testing 1st-order BLEP
  blep.vel_change_magnitude = 2.0; // apply a noticeable correction

  // Add directly to the active BLEPs for this block
  gen.currentActiveBlepOffsets.add(blep);

  const auto blampTable = audio_plugin::MinBlepGenerator::min_blep_deriv_array();
  const float expectedFirst = static_cast<float>(blep.vel_change_magnitude) *
                              blampTable.getUnchecked(0);

  gen.ProcessBlock(buffer, kNumSamples);

  // all samples should stay 0 (unmodified) until the blep
  for (int i = 0; i < 31; ++i) {
    ASSERT_EQ(buffer[i], 0.0f);
  }

  // The very first affected sample should match the scaled BLAMP start value
  EXPECT_NEAR(buffer[31], expectedFirst, 1.0e-5f);

  // For the rest, mirror the production stepping and interpolation logic for 1st-order correction
  for (int i = 32; i < kNumSamples; ++i) {
    // Gating uses the position branch stepping based on freqMultiple
    const double outputSamplesSinceBlep = static_cast<double>(blep.offset) + static_cast<double>(i) + 1.0;
    const double sampleExactGate = static_cast<double>(blep.freqMultiple) * outputSamplesSinceBlep;

    float expected = 0.0f;
    if (sampleExactGate >= 0.0) {
      // Derivative table stepping is depth-limited to proportionalBlepFreq
      const double derivExact = static_cast<double>(gen.proportional_blep_freq_) *
                                static_cast<double>(gen.over_sampling_ratio_) *
                                outputSamplesSinceBlep;
      double derivIdxD = 0.0;
      const double derivFrac = std::modf(derivExact, &derivIdxD);
      const int derivIdx = static_cast<int>(derivIdxD);

      if (derivIdx < blampTable.size()) {
        const float before = blampTable.getUnchecked(derivIdx);
        const float after = (derivIdx + 1 < blampTable.size())
                                ? blampTable.getUnchecked(derivIdx + 1)
                                : before;
        const float interp = before + static_cast<float>(derivFrac) * (after - before);
        expected = static_cast<float>(blep.vel_change_magnitude) * interp;
      } else {
        expected = 0.0f; // past end of table
      }
    }

    EXPECT_NEAR(buffer[i], expected, 1.0e-5f);
  }
}
}