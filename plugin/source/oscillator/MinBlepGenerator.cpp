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
#include <ranges>
#include <span>
#include <generator>

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

// todo below calculation seems sus - there is more straightforward impl in
// cardinal
//  that we could try to use instead. The generated minBlepArray doesn't seem
//  right - values are WAY too big. Could be rounding or precision error caused
//  by my changes?

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
  double maxVal =
      static_cast<double>(minBlepArray.getUnchecked(static_cast<int>(n - 1)));
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(),
                                        static_cast<float>(1.0 / maxVal), n);

  // Normalize ...
  float max = juce::FloatVectorOperations::findMaximum(
      minBlepDerivArray.getRawDataPointer(), n);
  // todo assert fails - problem?
  // jassert(fabs(static_cast<double>(max - minBlepDerivArray.getLast())) <
  // 0.0001);
  juce::FloatVectorOperations::multiply(minBlepDerivArray.getRawDataPointer(),
                                        1.0f / max, minBlepDerivArray.size());

  for (const auto ramp : std::views::iota(0, n)) {
    // 2ND ORDER ::::
    minBlepDerivArray.getRawDataPointer()[static_cast<int>(ramp)] -=
        static_cast<float>(ramp / static_cast<double>(n - 1));
  }

  DBG(min_blep_array().size());

  // todo assert fails here - problem?
  // jassert(fabsf(minBlepDerivArray[0]) < 0.01f);
  // todo assert here fails - problem?
  // jassert(fabsf(minBlepDerivArray[static_cast<int>(n - 1)]) < 0.01f);

  // SUBTRACT 1 and invert so the signal (so it goes 1->0)
  juce::FloatVectorOperations::add(minBlepArray.getRawDataPointer(), -1.f,
                                   minBlepArray.size());
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(), -1.f,
                                        minBlepArray.size());

  jassert(fabsf(minBlepArray[static_cast<int>(n)]) < 0.001f);

  dumpArrayToCsv(minBlepArray, "minbleparrNormSub.csv");
  dumpArrayToCsv(minBlepDerivArray, "minblepDevarrNormSub.csv");
}

// TODO: FIFO needs to ACCUMULATE bleps that START in the current buffer until we are done generating the audio
//  for the current buffer...not simply add values one after the other.
//  NOT sure if FIFO is the right DS for this - what we need is really just a ring buffer
//  (I have working example in sapf repo)
void MinBlepGenerator::AddBlep(const BlepOffset& newBlep) {
  ;
  const double blep_out_start_idx_exact = newBlep.offset;
  // todo: isn't there a bettter function for this?
  const double blep_out_start_idx_frac = blep_out_start_idx_exact - std::floor(blep_out_start_idx_exact);
  const int first_blep_out_idx = static_cast<int>(std::floor(blep_out_start_idx_exact));
  // ignore overly small bleps
  const auto hasPosChange = std::abs(newBlep.pos_change_magnitude) > 1e-9;
  const auto hasVelChange = std::abs(newBlep.vel_change_magnitude) > 1e-9;
  // we'll need this later. Since we never start exactly on the blep start,
  // this tells us how far into the oversampled blep table do we start
  // For example, if blep starts at out sample 5.33, that means the blep starts
  // being added only on our sample 6. At that point, we are already .66 (1-.33) of the way into
  // the blep (downsampled), = .66*kFreqMultiple samples into the oversampled blep table.
  // This simplifies the loop calculation - we can simply add kFreqMultiple * (iteration count) to this value.
  const auto blep_table_start_idx_exact = (1 - blep_out_start_idx_frac) * kFreqMultiple;
  // TODO: use template and loop the tables separately depending on the type of blep
  // todo: should be -1 for interp of last sample?
  // we start at 1 because the blep always starts somewhere between samples,
  // so we end up always starting to add the blep at the NEXT output sample from
  // where the blep ACTUALLY occurred.
  // For example if blep starts at out sample 3.34, sample 3 will have no blep, sample 4 WILL have blep,
  // so there's nothing to compute for sample 3
  // -1 because the last sample needs interpolation, so we need to stop one before the end of the blep table
  for (const int out_sample_offset : std::views::iota(1, kBlepOutLength-1)) {
    // what output buffer sample are we currently determining the output for?
    const int output_sample_idx = first_blep_out_idx + out_sample_offset;
    // where exactly are we within the blep for this output sample?
    // remember, the blep started BETWEEN output samples, so this will be between
    // indices within the (oversampled) blep table as well
    // For example -
    // If the blep occurred at out sample 5.33...
    // The FIRST sample we start adding blep will be 6. (out_sample_offset = 1, the first iteration of this loop).
    // The actual blep table position at that point will be .66 (downsampled)
    //    = .66*kFreqMultiple samples into the oversampled blep table.
    // On the next out sample, 7, we have moved +1 in the out sample buffer, but kFreqMultiple in the
    // oversampled blep table. So (.66*kFreqMultiple) + kFreqMultiple will be the exact position in the blep table.
    // On sample 8, it's (.66*kFreqMultiple) + kFreqMultiple*2, and then *3, and so on.
    const auto blep_table_idx_exact = (out_sample_offset - 1) * kFreqMultiple + blep_table_start_idx_exact;
    // we will need to interpolate between indices of the blep table
    const auto blep_table_idx_1 = static_cast<int>(blep_table_idx_exact);
    // this is the reason we iterate up to kBlepOutLength - 1, otherwise
    // this +1 would exceed the blep table bounds
    const auto blep_table_idx_2 = blep_table_idx_1 + 1;
    const auto blep_table_frac = blep_table_idx_exact - blep_table_idx_1;
    // blep start 5 - 5.33 - 6. blep table NaN - 0 - .33*8 = 2.64 So idx exact = 2.64, interp between table 2/3 (.33 + (out_idx - 1))) * 8
    // sample 5.33 - 6 - 7. at 6, we at 2.64. At 7, we + 8
    float correction = 0.0f;
    // 0th order
    if (hasPosChange) {
      const auto blep_sample_1 = minBlepArray[blep_table_idx_1];
      const auto blep_sample_2 = minBlepArray[blep_table_idx_2];
      const auto delta = blep_sample_2 - blep_sample_1;
      const auto val = blep_sample_1 + delta * blep_table_frac;
      correction += static_cast<float>(val * newBlep.pos_change_magnitude);
    }

    // 1st order
    if (hasVelChange) {
      // note the original impl claimed it limited this correction somehow,
      // but AFAICT the "limiting" actually had an identical result, it was
      // just using differently-named variables that ultimately produced
      // the same result
      const auto blep_sample_1 = minBlepDerivArray[blep_table_idx_1];
      const auto blep_sample_2 = minBlepDerivArray[blep_table_idx_2];
      const auto delta = blep_sample_2 - blep_sample_1;
      const auto val = blep_sample_1 + delta * blep_table_frac;
      correction += static_cast<float>(val * newBlep.vel_change_magnitude);
    }

    const int writePos = (read_index_ + output_sample_idx - 1) % kRingBufferSize;
    ring_buffer_[static_cast<size_t>(writePos)] += correction;
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
  if (std::abs(buffer[0]) < 0.00001f) return;
  for (int i = 0; i < numSamples; ++i) {
    const auto index = static_cast<size_t>((read_index_ + i) % kRingBufferSize);
    const float before = buffer[i];
    const float ring_val = ring_buffer_[index];
    buffer[i] += ring_val;
    const float after = buffer[i];
    ring_buffer_[index] = 0.0f;
    //todo: std::cout << before << "," << ring_val << "," << after << "\n";
  }

  // todo: should use addWithRamp to batch-add instead of above
  // todo: more efficient to batch clear ring_buffer after advancing read index

  read_index_ = (read_index_ + numSamples) % kRingBufferSize;
}

}  // namespace audio_plugin
