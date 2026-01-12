#pragma once
import JuceImports;
import std;

namespace audio_plugin {
// Because we generate at oversampled rate and don't use upsampling,
// juce's Oversampler cannot be used since it requires a very specific
// workflow that starts with upsampling.
// This class handles downsampling by any power of 2 using multi-stage
// polyphase IIR downsampling (adapted from juce's Oversampler).
class Downsampler {
public:
  void prepare(int max_block_size, int oversamplingFactor);

  void process(const juce::AudioBuffer<float> &input,
               juce::AudioBuffer<float> &output, int sourceStartSample,
               int sourceNumSamples);

private:
  struct Stage {
    std::vector<float> alphas;
    std::vector<float> v1;
    float delay { 0.0f };
  };

  std::vector<Stage> stages_;
  int oversamplingFactor_ { 1 };
  juce::AudioBuffer<float> internalBuffer_;
};
}