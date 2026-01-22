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
  // this determines how fast we step through the (oversampled) blep table
  // per output sample - it scales output samples into kernel samples (the
  // blep table is the kernel)
  constexpr double kFreqMultiple = kBlepOversampleRatio * kBlepProportionalFreq;
  // how long the blep should last for the current sample rate
  // blep lengths are the same - the blep is a bandlimited step (infinite freq)
  //  all that changes is how loud the blep is to counteract the step
  // TODO: sample rate can vary - shouldn't this not be constexpr?
  constexpr double kBlepLength = kBlepTableSize / kFreqMultiple;
  const double exactBlepOffset = newBlep.offset;
  // todo: isn't there a bettter function for this?
  const double blep_frac = exactBlepOffset - std::floor(exactBlepOffset);

  // todo: pointless vars - use directly the arrays
  const auto& blepTable = minBlepArray;
  const auto& derivTable = minBlepDerivArray;

  // TODO: missing blep depth limiting

  const int firstSample = static_cast<int>(std::floor(exactBlepOffset));

  // ignore overly small bleps
  const auto hasPosChange = std::abs(newBlep.pos_change_magnitude) > 1e-9;
  const auto hasVelChange = std::abs(newBlep.vel_change_magnitude) > 1e-9;
  // TODO: use template and loop the tables separately depending on the type of blep
  for (int i = 0; i < kBlepLength; ++i) {

    // TODO: need to make sure we are applying this old logic below comment
    // figure out how many output samples (p) have transpired
    // since the blep - this will be negative if we haven't yet reached the
    // blep. +1 because the blep needs to be mixed in starting on the LOW
    // SAMPLE

    const int outputSampleIdx = firstSample + i;
    // TODO: might be off by one errors here related to the lerp logic
    //convert to an index in the oversampled blep table
    const double currentBlepTableSampleExact = (i * kFreqMultiple) + blep_frac;

    jassert(currentBlepTableSampleExact < (blepTable.size() - 1));

    float correction = 0.0f;

    const int tableIdx = static_cast<int>(currentBlepTableSampleExact);
    const double frac = currentBlepTableSampleExact - tableIdx;

    // lerp between the blep table subsamples
    // 0th order
    if (hasPosChange) {
      const float val = blepTable[tableIdx] + static_cast<float>(frac * (blepTable[tableIdx + 1] - blepTable[tableIdx]));
      correction += val * static_cast<float>(newBlep.pos_change_magnitude);
    }

    // 1st order
    if (hasVelChange) {
      const float val = derivTable[tableIdx] + static_cast<float>(frac * (derivTable[tableIdx + 1] - derivTable[tableIdx]));
      correction += val * static_cast<float>(newBlep.vel_change_magnitude);
    }

    const int writePos = (read_index_ + outputSampleIdx) % kRingBufferSize;
    ring_buffer_[static_cast<size_t>(writePos)] += correction;
  }
}

// REAL TIME ::::: the core functions :::::
void MinBlepGenerator::ProcessBlock(float* buffer, int numSamples) {
  jassert(numSamples > 0);

  // PROCESS BLEPS :::::
  ProcessCurrentBleps(buffer, numSamples);

  // GRAB the final value ....
  // just in case there is a nonlinearity at sample 0 of the next block ...
  last_value_ = buffer[numSamples - 1];
}

void MinBlepGenerator::ProcessCurrentBleps(float* buffer,
                                           const int numSamples) {
  for (int i = 0; i < numSamples; ++i) {
    const size_t index = static_cast<size_t>((read_index_ + i) % kRingBufferSize);
    buffer[i] += ring_buffer_[index];
    ring_buffer_[index] = 0.0f;
  }

  read_index_ = (read_index_ + numSamples) % kRingBufferSize;
}

}  // namespace audio_plugin
