#include "BBSynth/Downsampler.h"

namespace audio_plugin {
void Downsampler::prepare(const double base_sample_rate, const int max_block_size) {
  // Low-pass filter at ~0.45 of base sample rate
  const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
      base_sample_rate * 2.0, // oversampled rate
      static_cast<float>(base_sample_rate) * 0.45f // cutoff frequency
      );

  filter_.coefficients = coeffs;
  filter_.prepare({base_sample_rate * 2.0, static_cast<juce::uint32>(max_block_size) * 2, 1});
}

// todo: pretty bad design - try to use same approach as Oversampling2TimesPolyphaseIIR instead
void Downsampler::process(const juce::AudioBuffer<float>& input,
                          juce::AudioBuffer<float>& output,
                          const int numOutputSamples) {
  auto* inputData = input.getReadPointer(0);
  auto* outputData = output.getWritePointer(0);

  for (int i = 0; i < numOutputSamples; ++i) {
    // Filter both oversampled samples
    filter_.processSample(inputData[i * 2]);
    outputData[i] = filter_.processSample(inputData[i * 2 + 1]);
  }
}
}