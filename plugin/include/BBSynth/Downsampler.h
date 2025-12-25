#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace audio_plugin {
// Because we generate at oversampled rate and don't use upsampling,
// juce's Oversampler cannot be used since it requires a very specific
// workflow that starts with upsampling.
// This class does just the downsampling / 2 using the same approach from juce's Oversampler.
class Downsampler {
public:
  void prepare(double base_sample_rate, int max_block_size);

  void process(const juce::AudioBuffer<float>& input,
               juce::AudioBuffer<float>& output,
               int numOutputSamples);

private:
  // Polyphase IIR downsampler state (adapted from juce::dsp::Oversampling 2x polyphase IIR down path)
  std::vector<float> alphasDown_;   // concatenated: direct-path alphas, then delayed-path alphas (without initial delay)
  std::vector<float> v1Down_;       // per-stage state for cascaded allpass filters
  float delayDown_ { 0.0f };        // single-sample delay for the delayed path
};
}