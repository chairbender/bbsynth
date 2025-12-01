/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/minBlepGenerator.cpp
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/
// todo rewrite to modern c++ standards
// todo cleanup / reduce need for static casting - some places are using size_t in places where the juce lib wants int.

#include "BBSynth/MinBlepGenerator.h"

#include <fstream>

namespace audio_plugin {

// STATIC ARRAYS - to house the minBlep and integral of the minBlep ...
// this array is used to handle POSITION discontinuities - 0th order - i.e. step response
static juce::Array<float> minBlepArray;
// this array is used to handle VELOCITY discontinuities - 1st order (effectively, BLAMP) i.e. ramp response.
// note that "deriv" refers to the fact that it CORRRECTS the derivative - it is not itself a derivative.
// In fact, it's actually the second integral of the minimum-phase impulse.
// This may as well have been called the blampArray.
static juce::Array<float> minBlepDerivArray;

// Helper: dump a numeric buffer to a CSV file in the user's Documents folder
static void dumpArrayToCsv(const juce::Array<double>& buffer, const juce::String& fileName)
{
  juce::File outFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile(fileName);
  std::ofstream csv(outFile.getFullPathName().toStdString(), std::ios::out | std::ios::trunc);
  if (!csv.is_open())
    return;

  csv << "index,value\n";
  const int count = buffer.size();
  for (int i = 0; i < count; ++i)
  {
    csv << i << "," << buffer.getUnchecked(i) << "\n";
  }
  csv.flush();
}

// todo use template to dedupe
static void dumpArrayToCsv(const juce::Array<float>& buffer, const juce::String& fileName)
{
  juce::File outFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile(fileName);
  std::ofstream csv(outFile.getFullPathName().toStdString(), std::ios::out | std::ios::trunc);
  if (!csv.is_open())
    return;

  csv << "index,value\n";
  const int count = buffer.size();
  for (int i = 0; i < count; ++i)
  {
    csv << i << "," << buffer.getUnchecked(i) << "\n";
  }
  csv.flush();
}

MinBlepGenerator::MinBlepGenerator() {
  overSamplingRatio = 16;
  zeroCrossings = 16;
  returnDerivative = false;
  proportionalBlepFreq = 0.5;  // defaults to NyQuist ....

  lastValue = 0;
  lastDelta = 0;

  // AA FILTER
  juce::zeromem(coefficients, sizeof(coefficients));

  numChannels = 2;
  filterStates.calloc(numChannels);

  ratio = 1;
  lastRatio = 1;

  createLowPass(ratio);
  resetFilters();

  buildBlep();
}
MinBlepGenerator::~MinBlepGenerator() {
  //
}

void MinBlepGenerator::setLimitingFreq(float proportionOfSamplingRate) {
  //
  // Instead of limiting to the sampling F,
  // We bring the maximum allowable F down to some known quantity
  // Doing this we can "tune" the blep to some desired F
  // So .... making wave-generators better ....

  // SINCE the buffer is only resized to 8x, we can only use blep adjustments
  // down to 0.125
  proportionOfSamplingRate =
      juce::jlimit<float>(0.0001f, 1.0f, proportionOfSamplingRate);
  proportionalBlepFreq = static_cast<double>(proportionOfSamplingRate);
}

juce::Array<float> MinBlepGenerator::getMinBlepArray() {
  return minBlepArray;
}
juce::Array<float> MinBlepGenerator::getMinBlepDerivArray() {
  return minBlepDerivArray;
}

void MinBlepGenerator::clear() {
  jassert(currentActiveBlepOffsets.size() == 0);

  currentActiveBlepOffsets.clear();
}
bool MinBlepGenerator::isClear() const {
  return currentActiveBlepOffsets.isEmpty();
}

// todo below calculation seems sus - there is more straightforward impl in cardinal
//  that we could try to use instead. The generated minBlepArray doesn't seem right - values
//  are WAY too big. Could be rounding or precision error caused by my changes?


// MIN BLEP - freq domain calc
void MinBlepGenerator::buildBlep() {
  // ALREADY built - so return ...
  if (minBlepArray.size() > 0)
    return;

  // BUILD the BLEP
  int i;
  juce::Array<double> buffer1;

  const auto n = static_cast<int>(zeroCrossings * 2 * overSamplingRatio);

  DBG("BUILD minBLEP - ratio " + juce::String(overSamplingRatio) + " -> " +
      juce::String(n));

  // Generate symmetric sinc array with specified number of
  // zero crossings on each side
  for (i = 0; i < n; i++) {
    // rescale from 0 - n-1 to -zeroCrossing to zeroCrossing
    const auto p = static_cast<float>(i) / static_cast<float>(n - 1)
      * ((static_cast<float>(zeroCrossings)*2)) - static_cast<float>(zeroCrossings);
    buffer1.add(SINC(static_cast<double>(p)));
  }

  jassert(buffer1.size() == static_cast<int>(n));

  dumpArrayToCsv(buffer1, "sinc.csv");

  // Window Sinc
  ApplyBlackmanHarrisWindow(n, buffer1.getRawDataPointer());

  dumpArrayToCsv(buffer1, "blackman.csv");

  // Minimum Phase Reconstruction
  RealCepstrum(static_cast<size_t>(n), buffer1.getRawDataPointer());
  dumpArrayToCsv(buffer1, "cepstrum.csv");
  MinimumPhase(static_cast<size_t>(n), buffer1.getRawDataPointer());

  dumpArrayToCsv(buffer1, "minphase.csv");

  // Integrate Into MinBLEP and BLAMP lookups
  minBlepArray.ensureStorageAllocated(static_cast<int>(n));
  minBlepDerivArray.ensureStorageAllocated(static_cast<int>(n));

  double a = 0;
  double secondInt = 0;
  for (i = 0; i < n; i++) {
    a += buffer1[static_cast<int>(i)];  // full integral ... so that we can normalize (make area=1)
    minBlepArray.add(static_cast<float>(a));

    // 2ND ORDER ::::
    secondInt += a;
    minBlepDerivArray.add(static_cast<float>(secondInt));
  }

  dumpArrayToCsv(minBlepArray, "minbleparr.csv");
  dumpArrayToCsv(minBlepDerivArray, "minblepDevarr.csv");

  // Normalize
  double maxVal = static_cast<double>(minBlepArray.getUnchecked(static_cast<int>(n - 1)));
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(),
                                  static_cast<float>(1.0 / maxVal), n);

  // Normalize ...
  float max = juce::FloatVectorOperations::findMaximum(
      minBlepDerivArray.getRawDataPointer(), n);
  // todo assert fails - problem?
  //jassert(fabs(static_cast<double>(max - minBlepDerivArray.getLast())) < 0.0001);
  juce::FloatVectorOperations::multiply(minBlepDerivArray.getRawDataPointer(),
                                  1.0f / max, minBlepDerivArray.size());

  for (double ramp = 0; ramp < static_cast<double>(n); ramp++) {
    // 2ND ORDER ::::
    // todo not sure what this does, but falling saw doesnt even use 2nd order corrections
    minBlepDerivArray.getRawDataPointer()[static_cast<int>(ramp)] -=
        static_cast<float>(ramp / static_cast<double>(n - 1));
  }

  // todo assert fails here - problem?
  //jassert(fabsf(minBlepDerivArray[0]) < 0.01f);
  // todo assert here fails - problem?
  //jassert(fabsf(minBlepDerivArray[static_cast<int>(n - 1)]) < 0.01f);

  // SUBTRACT 1 and invert so the signal (so it goes 1->0)
  juce::FloatVectorOperations::add(minBlepArray.getRawDataPointer(), -1.f,
                             minBlepArray.size());
  juce::FloatVectorOperations::multiply(minBlepArray.getRawDataPointer(), -1.f,
                                  minBlepArray.size());

  jassert(fabsf(minBlepArray[static_cast<int>(n)]) < 0.001f);


  dumpArrayToCsv(minBlepArray, "minbleparrNormSub.csv");
  dumpArrayToCsv(minBlepDerivArray, "minblepDevarrNormSub.csv");
}

void MinBlepGenerator::addBlep(BlepOffset newBlep) {
  jassert(newBlep.offset <= 0);

  newBlep.freqMultiple = overSamplingRatio * proportionalBlepFreq;
  currentActiveBlepOffsets.add(newBlep);
}

void MinBlepGenerator::addBlepArray(const juce::Array<BlepOffset>& newBleps) {
  currentActiveBlepOffsets.addArray(newBleps, 0, newBleps.size());
}

juce::Array<MinBlepGenerator::BlepOffset> MinBlepGenerator::getNextBleps() {
  juce::Array<MinBlepGenerator::BlepOffset> newBleps;
  newBleps.addArray(currentActiveBlepOffsets, 0,
                    currentActiveBlepOffsets.size());

  jassert(newBleps.size() == currentActiveBlepOffsets.size());

  // CLEAR the array ...
  currentActiveBlepOffsets.clearQuick();

  return newBleps;
}

// REAL TIME ::::: the core functions :::::
void MinBlepGenerator::processBlock(float* buffer, int numSamples) {
  // look for non-linearities ....
  jassert(numSamples > 0);

  // NON-LINEARITIES :::::
  // This is for processing detected nonlinearities about which we ONLY know the
  // POSITION process_nonlinearities(buffer, numSamples, nonlinearities);

  // GRAB the final value ....
  // just in case there is a nonlinearity at sample 0 of the next block ...
  // MUST be done BEFORE we ADD the bleps
  lastValue = buffer[numSamples - 1];

  // Hmmm .... once in a while there is a nonlinearity at the edge ....
  // inwhich case, we probably shouldn't update the delta ...
  // jassert(lastDelta == 0);
  jassert(currentActiveBlepOffsets.size() > 0);
  if (static_cast<int>(currentActiveBlepOffsets.getLast().offset) != -(numSamples - 1)) {
    lastDelta = buffer[numSamples - 1] - buffer[numSamples - 2];
  } else  // hacky .... hmmm ...
  {
    lastDelta = buffer[numSamples - 2] - buffer[numSamples - 3];
  }

  // PROCESS BLEPS :::::
  process_currentBleps(buffer, numSamples);
}

void MinBlepGenerator::rescale_bleps_to_buffer(const float* buffer,
                                               const int numSamples,
                                               const float shiftBlepsBy) {
  // MUST be big enough to hold the entire wave after all ... and safety factor
  // of 2
  jassert(currentActiveBlepOffsets.size() < 1000);

  for (int i = currentActiveBlepOffsets.size(); --i >= 0;) {
    // confusingly, the nonlinearities actually occued 1 sample BEFORE the
    // number we get this is because the detector detects that one JUST OCCURED,
    // and then adds it with the fractional offset from the last sample
    BlepOffset blep = currentActiveBlepOffsets.getUnchecked(i);  // ie 101.2

    // SCALE FREQ (to this bleps prop freq)
    blep.freqMultiple = overSamplingRatio * proportionalBlepFreq;

    // MODIFY :::: the exact offset ...
    // since this is an effect ... it manifests 1 sample later than the
    // discontinuity
    float exactOffset =
        static_cast<float>(-blep.offset + static_cast<double>(shiftBlepsBy));  // +1 here is NEEDED for flanger/chorus !!
    blep.offset =
        blep.offset -
        static_cast<double>(shiftBlepsBy);  // starts compensating on the sample AFTER the blep ....

    // ACTIVE blep .... (not upcoming)
    if (exactOffset < 0)
      continue;

    // CHECK :::: further away than 1 buffer ... should never happen
    if (exactOffset > static_cast<float>(numSamples)) {
      // LFOs have nonlinearities that affect the audio 1 sample later
      // ... so we can get edge cases here ....
      // simply roll it over to the next buffer ...
      DBG("OUT OF RANGE NONLINEARITY ??? " + juce::String(exactOffset));
      blep.offset = static_cast<double>(exactOffset - static_cast<float>(numSamples));
      currentActiveBlepOffsets.setUnchecked(i, blep);
      continue;
    }

    // CALCULATE the MAGNITUDE of the nonlinearity
    float magnitude_position = 0;
    float magnitude_velocity = 0;
    {
      float currentDelta = 0;
      // CALCULATE the integer (sample) offset, and the fractional (subsample)
      // offset
      double intOffset = static_cast<int>(exactOffset);
      double fraction = modf(static_cast<double>(exactOffset), &intOffset);

      // UNLESS we're on the edge case, we get the most recent value from the
      // buffer ...
      if (intOffset > 0)
        lastValue = buffer[static_cast<int>(intOffset) - 1];

      // 1st order (velocity)
      // MUST do this one first - since the 0th order may change the LastValue
      {
        // FIND the last and next deltas ... and compute the difference ....
        if (intOffset >= 2)
          lastDelta = buffer[static_cast<int>(intOffset) - 1] - buffer[static_cast<int>(intOffset) - 2];
        else if (intOffset >= 1)
          lastDelta = buffer[static_cast<int>(intOffset) - 1] - lastValue;

        // DEFAULT :: assume flat ...
        currentDelta = 0;

        if (intOffset + 1 < numSamples)
          currentDelta = buffer[static_cast<int>(intOffset) + 1] - buffer[static_cast<int>(intOffset)];

        // CALCULATE change in velocity
        double change_in_delta = static_cast<double>(currentDelta - lastDelta);
        double propDepth = proportionalBlepFreq;

        // magnitude_velocity = -4*change_in_delta*(1/propDepth);
        // jassert(magnitude_velocity <= 1);

        // actualCurrentAngleDelta below is added to compensate for higher order
        // nonlinearities 66 here was experimentally determined ...
        magnitude_velocity =
            static_cast<float>(64.7 * change_in_delta * (1 / propDepth) * change_in_delta);
      }

      // 0th order (position)
      {
        // CALCULATE the magnitude of the 0 order nonlinearity *change in
        // position*
        float extrapolated_last_pos = static_cast<float>(static_cast<double>(lastValue + lastDelta) * fraction);
        float extrapolated_jump_pos =
            static_cast<float>(static_cast<double>(buffer[static_cast<int>(intOffset)] - currentDelta) * (1 - fraction));
        magnitude_position = extrapolated_last_pos - extrapolated_jump_pos;
      }
    }

    // TOO SMALL :::
    /// no need to compensating for tiny discontinuities
    if (fabsf(magnitude_position) < .001f && fabsf(magnitude_velocity) < .001f) {
      currentActiveBlepOffsets.remove(i);
      continue;
    }

    // NEGLIGIBLE MAGNITUDES :::
    /// zero out any tiny effects here, so we don't waste time calculating them
    if (fabsf(magnitude_position) < .001f)
      magnitude_position = 0;
    if (fabsf(magnitude_velocity) < .001f)
      magnitude_velocity = 0;

    // ADD ::::
    // GAIN factors ... how big of a discontinutiy are we talking about ?
    blep.pos_change_magnitude = static_cast<double>(magnitude_position);
    blep.vel_change_magnitude = static_cast<double>(magnitude_velocity);

    // ALTER :::
    currentActiveBlepOffsets.setUnchecked(i, blep);
  }
}

/**
 * Applies a correction, looked up from the appropriate blep or blamp
 * correction table (oversampled), at a specified index + subsample within
 * that table, using lerp to interp between the samples of the correction table.
 * The correction is added to the given outputSampleIdx within outputBuffer.
 */
static void lerpCorrection(float* outputBuffer,
  const juce::Array<float>& correctionTable,
  const int correctionSample, const double correctionSubSample,
  const double discontinuityMagnitude,
  const int outputSampleIdx) {
  // We have the subsample within the blep table, not just the sample, so we'll use that with linear interpolation
  // to ensure we get an even more accuracte blep value to mix in.
  const float blepSampleBefore = correctionTable.getRawDataPointer()[correctionSample];
  float blepSampleAfter = blepSampleBefore;

  if (static_cast<int>(correctionSample) + 1 < correctionTable.size())
    blepSampleAfter = correctionTable.getRawDataPointer()[static_cast<int>(correctionSample) + 1];

  const float delta = blepSampleAfter - blepSampleBefore;
  float exactValue = static_cast<float>(static_cast<double>(blepSampleBefore) + correctionSubSample * static_cast<double>(delta));

  // SCALE by the discontinuity magnitude
  exactValue *= static_cast<float>(discontinuityMagnitude);

  // ADD to the thruput
  outputBuffer[static_cast<int>(outputSampleIdx)] += exactValue;
}

void MinBlepGenerator::process_currentBleps(float* buffer, int numSamples) {
  // PROCESS ALL BLEPS -
  /// for each offset, mix a portion of the blep array with the output ....
  for (int i = currentActiveBlepOffsets.size(); --i >= 0;) {
    BlepOffset blep = currentActiveBlepOffsets[i];

    // this determines how fast we step through the (oversampled) blep table
    // per output sample - it scales output samples into kernel samples (the blep table is the kernel)
    const double freqMultiple = blep.freqMultiple;
    // remember this will be negative when the blep occurred this buffer (as opposed to a recent previous buffer)
    // and the magnitude (ignoring the negative sign) is the index it occurred at in this buffer.
    const double exactBlepOffset = blep.offset;

    // mix the minBLEP table into the buffer at the point where the blep occurs
    // within the current buffer.
    // To do this, we first have to step through the current buffer until we reach a point
    // where the blep has occurred (which will already be true if the blep happened in a recent previous
    // buffer).
    for (float p = 0; p < static_cast<float>(numSamples); p++) {

      // figure out how many output samples (p) have transpired
      // since the blep - this will be negative if we haven't yet reached the blep.
      // +1 because the blep needs to be mixed in starting on the LOW SAMPLE
      const auto outputSamplesSinceBlep = exactBlepOffset + static_cast<double>(p) + 1;

      // by scaling by freqMultiple, we convert to a lookup on the blep table (which is oversampled).
      // Think of it as a "blep sample".
      // It's a double because we also preserve the fractional part (subsample).
      // Again, this number is negative and basically meaningless if we haven't yet reached the blep
      // todo we shouldn't even bother to calculate until we've passed the blep
      const double currentBlepTableSampleExact = freqMultiple * outputSamplesSinceBlep;
      double currentBlepTableSample = 0;
      // the fractional part of the "blep sample" we need.
      const double currentBlepTableSubSample = modf(currentBlepTableSampleExact, &currentBlepTableSample);

      // LIMIT the correction applied for velocity discontinuity
      // otherwise, it can get TOO large (for example at high freqs) and ends up over-correcting.
      // This limiting is done by reducing how fast we advance through the BLAMP table - adjusting
      // the scaling factor that converts from output sample idx to blamp lookup idx.
      const double depthLimited =
          proportionalBlepFreq;  // jlimit<double>(.1, 1, proportionalBlepFreq);
      const double currentBlepDerivTableSampleExact =
          depthLimited * overSamplingRatio * (exactBlepOffset + static_cast<double>(p) + 1);
      double currentBlepDerivTableSample = 0;
      const double currentBlepDerivTableSubSample = modf(currentBlepDerivTableSampleExact, &currentBlepDerivTableSample);

      // DONE ... we reached the place where this blep should end (which may be after multiple bufferfulls
      // of applying the blep)
      if (static_cast<int>(currentBlepTableSampleExact) > minBlepArray.size() &&
          static_cast<int>(currentBlepDerivTableSampleExact) > minBlepArray.size())
        break;

      // BLEP has not yet occurred ...
      if (currentBlepTableSampleExact < 0)
        continue;

      // 0TH ORDER (POSITION DISCONTINUITY) COMPENSATION ::::
      // we reached the location of this blep or we are somewhere after it
      // We need to mix in the corresponding value in the blep table. E.g.
      // if blep occurred 3 samples ago we should mix in sample 3 of the blep table.
      // But that's not quite the case - the blep table is oversampled, so we need to
      // convert the index to one on the blep table (using the subsample information we preserved about the blep).
      // We also don't want to mix the blep in at its full value - we scale the blep based on how big the
      // original blep discontinuity was.
      if (fabs(blep.pos_change_magnitude) > 0 &&
          currentBlepTableSample < minBlepArray.size()) {
        lerpCorrection(buffer, minBlepArray,
          static_cast<int>(currentBlepTableSample),
          currentBlepTableSubSample, blep.pos_change_magnitude,
          static_cast<int>(p));
      }

      // 1ST ORDER COMPENSATION ::::
      /// add the BLEP DERIVATIVE to compensate for discontinuties in the
      /// VELOCITY - this is BLAMP basically.
      if (fabs(blep.vel_change_magnitude) > 0 &&
          currentBlepDerivTableSampleExact < minBlepDerivArray.size()) {

        lerpCorrection(buffer, minBlepDerivArray,
          static_cast<int>(currentBlepDerivTableSample), currentBlepDerivTableSubSample,
          blep.vel_change_magnitude, static_cast<int>(p));
      }
    }

    // UPDATE ::::
    blep.offset = blep.offset + static_cast<double>(numSamples);
    if (blep.offset * freqMultiple > minBlepArray.size()) {
      currentActiveBlepOffsets.remove(i);
    } else
      currentActiveBlepOffsets.setUnchecked(i, blep);
  }
}

}  // namespace audio_plugin
