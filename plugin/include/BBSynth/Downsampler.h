#pragma once

#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {
// Because we generate at oversampled and don't use upsampling,
// juce's Oversampler cannot be used since it requires a very specific
// workflow.
// This class does just the downsampling using our own approach.
// It does a single /2 downsampling.
class Downsampler {
public:
  void prepare(double base_sample_rate, int max_block_size);

  void process(const juce::AudioBuffer<float>& input,
               juce::AudioBuffer<float>& output,
               int numOutputSamples);

private:
  juce::dsp::IIR::Filter<float> filter_;
};
}