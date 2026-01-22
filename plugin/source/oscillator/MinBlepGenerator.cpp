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

MinBlepGenerator::MinBlepGenerator() : fifo_{kRingBufferSize} {
  ring_buffer_.fill(0.0f);
  over_sampling_ratio_ = 16;
  zero_crossings_ = 16;
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
  fifo_.reset();
}
bool MinBlepGenerator::IsClear() const {
  return fifo_.getNumReady() == 0;
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

  const auto n = static_cast<int>(zero_crossings_ * 2 * over_sampling_ratio_);

  DBG("BUILD minBLEP - ratio " + juce::String(over_sampling_ratio_) + " -> " +
      juce::String(n));

  // Generate symmetric sinc array with specified number of
  // zero crossings on each side
  for (const auto i : std::views::iota(0, n)) {
    // rescale from 0 - n-1 to -zeroCrossing to zeroCrossing
    const auto p = static_cast<float>(i) / static_cast<float>(n - 1) *
                       ((static_cast<float>(zero_crossings_) * 2)) -
                   static_cast<float>(zero_crossings_);
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

void MinBlepGenerator::AddBlep(BlepOffset newBlep) {
  const double freqMultiple = over_sampling_ratio_ * proportional_blep_freq_;
  const double exactBlepOffset = newBlep.offset;

  const auto& blepTable = minBlepArray;
  const auto& derivTable = minBlepDerivArray;

  const int maxLength = static_cast<int>(static_cast<double>(blepTable.size()) / freqMultiple) + 1;

  int start1, size1, start2, size2;
  fifo_.prepareToWrite(kRingBufferSize, start1, size1, start2, size2);
  const int writeIndex = start1;

  for (int p = 0; p < maxLength; ++p) {
    const double outputSamplesSinceBlep = exactBlepOffset + static_cast<double>(p) + 1;
    if (outputSamplesSinceBlep < 0) continue;

    const double currentBlepTableSampleExact = freqMultiple * outputSamplesSinceBlep;
    const double depthLimited = proportional_blep_freq_;
    const double currentBlepDerivTableSampleExact = depthLimited * over_sampling_ratio_ * outputSamplesSinceBlep;

    bool blepValid = false;
    float correction = 0.0f;

    // 0th order
    int tableIdx = static_cast<int>(currentBlepTableSampleExact);
    if (std::abs(newBlep.pos_change_magnitude) > 0 && tableIdx < blepTable.size() - 1) {
      double frac = currentBlepTableSampleExact - tableIdx;
      float val = blepTable[tableIdx] + static_cast<float>(frac * (blepTable[tableIdx + 1] - blepTable[tableIdx]));
      correction += val * static_cast<float>(newBlep.pos_change_magnitude);
      blepValid = true;
    }

    // 1st order
    int derivIdx = static_cast<int>(currentBlepDerivTableSampleExact);
    if (std::abs(newBlep.vel_change_magnitude) > 0 && derivIdx < derivTable.size() - 1) {
      double frac = currentBlepDerivTableSampleExact - derivIdx;
      float val = derivTable[derivIdx] + static_cast<float>(frac * (derivTable[derivIdx + 1] - derivTable[derivIdx]));
      correction += val * static_cast<float>(newBlep.vel_change_magnitude);
      blepValid = true;
    }

    if (!blepValid && p > 10) break;

    if (correction != 0.0f) {
      int writePos = (writeIndex + p) % kRingBufferSize;
      ring_buffer_[static_cast<size_t>(writePos)] += correction;
    }
  }

  int currentReady = fifo_.getNumReady();
  if (maxLength > currentReady) {
    int toAdd = maxLength - currentReady;
    toAdd = std::min(toAdd, fifo_.getFreeSpace());
    fifo_.finishedWrite(toAdd);
  }
}

void MinBlepGenerator::AddBlepArray(const juce::Array<BlepOffset>& newBleps) {
  for (const auto& blep : newBleps) {
    AddBlep(blep);
  }
}

// REAL TIME ::::: the core functions :::::
void MinBlepGenerator::ProcessBlock(float* buffer, int numSamples) {
  // look for non-linearities ....
  jassert(numSamples > 0);

  // NON-LINEARITIES :::::
  // This is for processing detected nonlinearities about which we ONLY know the
  // POSITION process_nonlinearities(buffer, numSamples, nonlinearities);

  // GRAB the final value ....
  // just in case there is a nonlinearity at sample 0 of the next block ...
  // MUST be done BEFORE we ADD the bleps
  last_value_ = buffer[numSamples - 1];

  // PROCESS BLEPS :::::
  ProcessCurrentBleps(buffer, numSamples);
}

void MinBlepGenerator::RescaleBlepsToBuffer(const float*, const int, const float) {}

void MinBlepGenerator::ProcessCurrentBleps(float* buffer,
                                           const int numSamples) {
  const int available = fifo_.getNumReady();
  if (available == 0) return;

  const int to_read = std::min(available, numSamples);
  int start1, size1, start2, size2;
  fifo_.prepareToRead(to_read, start1, size1, start2, size2);

  if (size1 > 0) {
    for (int i = 0; i < size1; ++i) {
      buffer[i] += ring_buffer_[static_cast<size_t>(start1 + i)];
      ring_buffer_[static_cast<size_t>(start1 + i)] = 0.0f;
    }
  }
  if (size2 > 0) {
    for (int i = 0; i < size2; ++i) {
      buffer[size1 + i] += ring_buffer_[static_cast<size_t>(start2 + i)];
      ring_buffer_[static_cast<size_t>(start2 + i)] = 0.0f;
    }
  }

  fifo_.finishedRead(to_read);
}

}  // namespace audio_plugin
