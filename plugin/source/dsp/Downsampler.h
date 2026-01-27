#pragma once

#include "../Constants.h"
#include "../juce_modified/juce_Oversampling.h"

/**
 * Simple convenience wrapper for our modified OversamplingDownsampling
 * class.
 * Workflow:
 * - Prepare in prepareToPlay to set max block size.
 * - Get oversampled buffer with GetOverSampleBuffer.
 * - Write oversampled audio into buffer
 * - Downsample with Downsample, passing the buffer back to Downsampler.
 */
namespace audio_plugin {
class Downsampler {
public:
  void Prepare(int max_block_size);
  juce::dsp::AudioBlock<const float> GetOverSampleBuffer(
      const juce::dsp::ProcessContextReplacing<float> &context);

  void Downsample(const juce::dsp::ProcessContextReplacing<float> &context);

private:
  // TODO: make below parameterized, so you can try different oversampling methods and factor as well
  //  as switching to max quality when desired.
  juce::dsp::OversamplingDownsampling<float> downsampler_{1, kOversample,
  juce::dsp::OversamplingDownsampling<float>::filterHalfBandFIREquiripple, false};

};
}