/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/minBlepGenerator.cpp
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/
// todo rewrite to modern c++ standards
// todo cleanup / reduce need for static casting - some places are using size_t
// in places where the juce lib wants int.

#include "MinBlepGenerator.h"

#include <fstream>
#include <generator>
#include <ranges>
#include <span>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

// STATIC ARRAYS - to house the minBlep and integral of the minBlep ...
// this array is used to handle POSITION discontinuities - 0th order - i.e. step
// response
static juce::Array<float> minBlepArray;
// this array is used to handle VELOCITY discontinuities - 1st order
// (effectively, BLAMP) i.e. ramp response. note that "deriv" refers to the fact
// that it CORRRECTS the derivative - it is not itself a derivative. In fact,
// it's actually the second integral of the minimum-phase impulse. This may as
// well have been called the blampArray.
static juce::Array<float> minBlepDerivArray;

// todo use template to dedupe
template <typename T>
static void dumpArrayToCsv(const juce::Array<T>& buffer,
                           const juce::String& fileName) {
  juce::File outFile =
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
          .getChildFile(fileName);
  std::ofstream csv(outFile.getFullPathName().toStdString(),
                    std::ios::out | std::ios::trunc);
  if (!csv.is_open()) return;

  csv << "index,value\n";
  for (const auto [index, sample] : std::views::enumerate(
           std::span(buffer.getRawDataPointer(), buffer.size()))) {
    csv << index << "," << sample << "\n";
           }
  csv.flush();
}

MinBlepGenerator::MinBlepGenerator() : read_index_{0} {
  ring_buffer_.fill(0.0f);
  return_derivative_ = false;
  proportional_blep_freq_ = 0.5;  // defaults to NyQuist ....

  last_value_ = 0;
  last_delta_ = 0;

  // AA FILTER
  juce::zeromem(coefficients_, sizeof(coefficients_));

  num_channels_ = 2;
  filter_states_.calloc(num_channels_);

  ratio_ = 1;
  last_ratio_ = 1;

  CreateLowPass(ratio_);
  ResetFilters();

  BuildBlep();
}
MinBlepGenerator::~MinBlepGenerator() {
  //
}



void MinBlepGenerator::set_limiting_freq(float proportionOfSamplingRate) {
  //
  // Instead of limiting to the sampling F,
  // We bring the maximum allowable F down to some known quantity
  // Doing this we can "tune" the blep to some desired F
  // So .... making wave-generators better ....

  // SINCE the buffer is only resized to 8x, we can only use blep adjustments
  // down to 0.125
  proportionOfSamplingRate =
      juce::jlimit<float>(0.0001f, 1.0f, proportionOfSamplingRate);
  proportional_blep_freq_ = static_cast<double>(proportionOfSamplingRate);
}
void MinBlepGenerator::set_aa_key_scaling(const bool enable) {
  aa_scaling_ = enable;
}

juce::Array<float> MinBlepGenerator::min_blep_array() { return minBlepArray; }
juce::Array<float> MinBlepGenerator::min_blep_deriv_array() {
  return minBlepDerivArray;
}

void MinBlepGenerator::Clear() {
  ring_buffer_.fill(0.0f);
  read_index_ = 0;
}
bool MinBlepGenerator::IsClear() const {
  for (const auto& sample : ring_buffer_) {
    if (std::abs(sample) > 1e-6f) return false;
  }
  return true;
}

// MIN BLEP - freq domain calc
void MinBlepGenerator::BuildBlep() const {
  // ALREADY built - so return ...
  if (minBlepArray.size() > 0) return;

  // BUILD the BLEP
  juce::Array<double> buffer1;

  constexpr auto n = static_cast<int>(kBlepZeroCrossings * 2 * kBlepOversampleRatio);

  DBG("BUILD minBLEP - ratio " + juce::String(kBlepOversampleRatio) + " -> " +
      juce::String(n));

  // Generate symmetric sinc array with specified number of
  // zero crossings on each side
  for (const auto i : std::views::iota(0, n)) {
    // rescale from 0 - n-1 to -zeroCrossing to zeroCrossing
    const auto p = static_cast<float>(i) / static_cast<float>(n - 1) *
                       ((static_cast<float>(kBlepZeroCrossings) * 2)) -
                   static_cast<float>(kBlepZeroCrossings);
    buffer1.add(Sinc(static_cast<double>(p)));
  }

  jassert(buffer1.size() == static_cast<int>(n));

  dumpArrayToCsv(buffer1, "sinc.csv");

  // Window Sinc
  ApplyBlackmanHarrisWindow(std::span(buffer1.getRawDataPointer(), n));

  dumpArrayToCsv(buffer1, "blackman.csv");

  // Minimum Phase Reconstruction
  RealCepstrum(std::span(buffer1.getRawDataPointer(), static_cast<size_t>(n)));
  dumpArrayToCsv(buffer1, "cepstrum.csv");
  MinimumPhase(std::span(buffer1.getRawDataPointer(), static_cast<size_t>(n)));

  dumpArrayToCsv(buffer1, "minphase.csv");

  // Integrate Into MinBLEP and BLAMP lookups
  minBlepArray.ensureStorageAllocated(static_cast<int>(n));
  minBlepDerivArray.ensureStorageAllocated(static_cast<int>(n));

  double a = 0;
  double secondInt = 0;
  for (const auto i : std::views::iota(0, n)) {
    a += buffer1[static_cast<int>(
        i)];  // full integral ... so that we can normalize (make area=1)
    minBlepArray.add(static_cast<float>(a));

    // 2ND ORDER ::::
    secondInt += a;
    minBlepDerivArray.add(static_cast<float>(secondInt));
  }

  dumpArrayToCsv(minBlepArray, "minbleparr.csv");
  dumpArrayToCsv(minBlepDerivArray, "minblepDevarr.csv");

  // Normalize
  const double maxVal =
      static_cast<double>(minBlepArray.getUnchecked(static_cast<int>(n - 1)));
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(),
                                        static_cast<float>(1.0 / maxVal), n);

  // Normalize ...
  const float max = juce::FloatVectorOperations::findMaximum(
      minBlepDerivArray.getRawDataPointer(), n);
  juce::FloatVectorOperations::multiply(minBlepDerivArray.getRawDataPointer(),
                                        1.0f / max, minBlepDerivArray.size());

  for (const auto ramp : std::views::iota(0, n)) {
    // 2ND ORDER ::::
    minBlepDerivArray.getRawDataPointer()[static_cast<int>(ramp)] -=
        static_cast<float>(ramp / static_cast<double>(n - 1));
  }

  DBG(min_blep_array().size());

  // SUBTRACT 1 and invert so the signal (so it goes 1->0)
  juce::FloatVectorOperations::add(minBlepArray.getRawDataPointer(), -1.f,
                                   minBlepArray.size());
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(), -1.f,
                                        minBlepArray.size());

  jassert(fabsf(minBlepArray[static_cast<int>(n)]) < 0.001f);

  dumpArrayToCsv(minBlepArray, "minbleparrNormSub.csv");
  dumpArrayToCsv(minBlepDerivArray, "minblepDevarrNormSub.csv");
}
void MinBlepGenerator::ApplyBlep(const int blep_out_length,
  const int first_blep_out_idx,
  const double freq_multiple,
  const double blep_table_start_idx_exact,
  const double magnitude,
  const juce::Array<float>& lookup) {
  for (const int out_sample_offset : std::views::iota(0, blep_out_length)) {
    // what output buffer sample are we currently determining the output for?
    const int output_sample_idx = first_blep_out_idx + out_sample_offset;
    // where exactly are we within the blep for this output sample?
    // Following the example, the out sample 30 should start
    // with the blep table value at index (interpolated) .66 * freq_multiple.
    // On sample 31, it should be (.66*freq_multiple) + freq_multiple, and so on
    // sample 32, .66 * freq_multiple + 2*freq_multiple and so on.
    const auto blep_table_idx_exact = out_sample_offset * freq_multiple + blep_table_start_idx_exact;
    // we will need to interpolate between indices of the blep table, so
    // let's calculate the indices
    const auto blep_table_idx_1 = static_cast<int>(blep_table_idx_exact);
    // at the end of the table, we stop interpolating
    // todo: not sure if this is the optimal edge case behavior, but seems to work fine
    const auto blep_table_idx_2 = std::min(blep_table_idx_1 + 1, kBlepTableSize - 1);
    const auto blep_table_frac = blep_table_idx_exact - blep_table_idx_1;
    float correction = 0.0f;
    const auto blep_sample_1 = lookup[blep_table_idx_1];
    const auto blep_sample_2 = lookup[blep_table_idx_2];
    const auto delta = blep_sample_2 - blep_sample_1;
    const auto val = blep_sample_1 + delta * blep_table_frac;
    correction += static_cast<float>(val * magnitude);

    const int writePos = (read_index_ + output_sample_idx) % kRingBufferSize;
    ring_buffer_[static_cast<size_t>(writePos)] += correction;
  }
}


void MinBlepGenerator::AddBlep(const BlepOffset& newBlep) {
  // this determines how fast we step through the (oversampled) blep table
  // per output sample - it scales output samples into kernel samples (the
  // blep table is the kernel).
  // We give the ability to have it respond to the frequency of the oscillator,
  // which slightly attenuates the higher partials of the sound.
  // When it's off, it stays fixed at the nyquist.
  // My understanding is, having it on supposedly makes it sound more "analog",
  // imitating the limitations of analog hardware such as op-amp slew rate limits.
  // IMHO, when it's on, it sounds more like a cheap children's toy, but
  // that's why we make it a parameter! (It's entirely possible I've implemented it wrong)
  const auto freq_multiple = kBlepOversampleRatio * (aa_scaling_ ? proportional_blep_freq_ : .5);
  // how long the blep should last for the current sample rate
  // blep lengths are the same - the blep is a bandlimited step (infinite freq)
  //  all that changes is how loud the blep is to counteract the step
  const int blep_out_length = static_cast<int>(kBlepTableSize / freq_multiple);

  // for the blep to work correctly, say it happens at sample 30.33.
  // The blep signal needs to be mixed in starting at 29.33 (it has a 1 sample
  // "anticipation" of the step). We can't put something between the sample,
  // so what we do instead is mix in starting at 30. At sample 30,
  // the blep signal should be .66 (1 - .33) of the way through its table (downsampled).
  // so in that example, the below would end up as 29.33.
  // Keep in mind that the blep table is actually oversampled, so saying
  // ".33 downsampled" actually means .33 * freq_multiple. And
  // even though the table is discrete, we use the decimal portion
  // of the "index" to help interpolate between blep table values.
  // to prevent issues that happen around offsets at sample 0,
  // we add 1 to some values then subtract when needed
  constexpr auto avoid_negative_offset = 1;
  const double blep_out_start_idx_exact = newBlep.offset - 1 + avoid_negative_offset;
  double first_blep_out_idx;
  const auto blep_out_start_idx_frac = std::modf(blep_out_start_idx_exact, &first_blep_out_idx);
  // in the example, this ends up as 30 due to the + 1, which is where we want to start
  // mixing in the blep signal
  // (at blep table sample (downsampled) index .66)
  // we don't need to do anything since offset is already +1.
  // first_blep_out_idx = first_blep_out_idx + 1 - avoid_negative_offset -i.e. no-op;
  // Saves some calculation. As noted in the example, if blep occurs at
  // 30.33, then it should "start" on sample 29.33, but the first actual
  // output we produce for the blep will be at sample 30, and we will be at
  // .66 (1 - .33) * freq_multiple (to make it upsampled) into the table.
  // From there on, we advance + freq_multiple for every out sample through the table
  // This simplifies the loop calculation - we can simply add freq_multiple * (iteration count) to this value.
  const auto blep_table_start_idx_exact = (1 - blep_out_start_idx_frac) * freq_multiple;

  if (newBlep.pos_change_magnitude > 1e-9) {
    ApplyBlep(blep_out_length, static_cast<int>(first_blep_out_idx),
      freq_multiple, blep_table_start_idx_exact, newBlep.pos_change_magnitude, minBlepArray);
  }

  // TODO: some depth limiting should maybe be done here by applying the
  //   proportional freq scaling being applied twice. I'm struggling
  //  to follow the original.
  if (newBlep.vel_change_magnitude > 1e-9) {
    ApplyBlep(blep_out_length, static_cast<int>(first_blep_out_idx),
      freq_multiple, blep_table_start_idx_exact, newBlep.vel_change_magnitude, minBlepDerivArray);
  }
}

// REAL TIME ::::: the core functions :::::
void MinBlepGenerator::ProcessBlock(float* buffer, const int numSamples) {
  jassert(numSamples > 0);

  // PROCESS BLEPS :::::
  ProcessCurrentBleps(buffer, numSamples);

  // GRAB the final value ....
  // just in case there is a nonlinearity at sample 0 of the next block ...
  // TODO: I'm not sure we actually need this...
  last_value_ = buffer[numSamples - 1];
}

void MinBlepGenerator::ProcessCurrentBleps(float* buffer,
                                           const int numSamples) {
  // since it's a ring buffer, we might go past the end,
  // so we may need to split this into 2 parts
  // part 1
  const auto ring_start = ring_buffer_.data() + read_index_;
  const auto ring_samples_remaining = kRingBufferSize - read_index_;
  const auto num_samples_part_1 = std::min(ring_samples_remaining, numSamples);
  juce::FloatVectorOperations::add(buffer, ring_start, num_samples_part_1);
  juce::FloatVectorOperations::clear(ring_start, num_samples_part_1);

  // part 2 if needed
  if (ring_samples_remaining < numSamples) {
    const auto num_samples_part_2 = numSamples - num_samples_part_1;
    const auto ring_start_2 = ring_buffer_.data();
    juce::FloatVectorOperations::add(buffer + num_samples_part_1, ring_start_2,
      num_samples_part_2);
    juce::FloatVectorOperations::clear(ring_start_2, num_samples_part_2);
  }

  read_index_ = (read_index_ + numSamples) % kRingBufferSize;
}

}  // namespace audio_plugin
