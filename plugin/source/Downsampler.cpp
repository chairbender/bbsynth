#include "BBSynth/Downsampler.h"

namespace audio_plugin {
void Downsampler::prepare(const double /*base_sample_rate*/, const int /*max_block_size*/) {
  // Use the same design approach as juce::dsp::Oversampling 2x polyphase IIR (down path)
  constexpr float twDown = 0.06f;   // normalised transition width for first stage (maximum quality path)
  constexpr float gaindBDown = -75.0f; // stopband attenuation for first stage (maximum quality path)

  auto structureDown = juce::dsp::FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod(twDown, gaindBDown);

  // Collect alpha coefficients: direct path allpass alphas first
  alphasDown_.clear();
  alphasDown_.reserve(static_cast<size_t>(structureDown.directPath.size() + structureDown.delayedPath.size()));

  for (int i = 0; i < structureDown.directPath.size(); ++i)
    alphasDown_.push_back(structureDown.directPath.getObjectPointer(i)->coefficients[0]);

  // Then delayed path allpass alphas, skipping the first element which is the pure delay stage
  for (int i = 1; i < structureDown.delayedPath.size(); ++i)
    alphasDown_.push_back(structureDown.delayedPath.getObjectPointer(i)->coefficients[0]);

  // Prepare state
  v1Down_.assign(alphasDown_.size(), 0.0f);
  delayDown_ = 0.0f;
}

void Downsampler::process(const juce::AudioBuffer<float>& input,
                          juce::AudioBuffer<float>& output,
                          const int numOutputSamples) {
  auto* inputData = input.getReadPointer(0);
  auto* outputData = output.getWritePointer(0);

  // Polyphase IIR downsampling (mono) adapted from juce::dsp::Oversampling2TimesPolyphaseIIR::processSamplesDown
  const auto numStages = static_cast<int>(alphasDown_.size());
  if (numStages == 0)
  {
    // Fallback: simple copy of every other sample if prepare() not called
    for (int i = 0; i < numOutputSamples; ++i)
      outputData[i] = 0.5f * (inputData[i * 2] + inputData[i * 2 + 1]);
    return;
  }

  const int delayedStages = numStages / 2;
  const int directStages  = numStages - delayedStages;

  float* lv1 = v1Down_.data();
  float delay = delayDown_;

  for (int i = 0; i < numOutputSamples; ++i) {
    // Direct path cascaded allpass filters (even sample)
    float inEven = inputData[(i << 1)];
    for (int n = 0; n < directStages; ++n) {
      const float alpha = alphasDown_[static_cast<size_t>(n)];
      const float out = alpha * inEven + lv1[n];
      lv1[n] = inEven - alpha * out;
      inEven = out;
    }
    const float directOut = inEven;

    // Delayed path cascaded allpass filters (odd sample)
    float inOdd = inputData[(i << 1) + 1];
    for (int n = directStages; n < numStages; ++n) {
      const float alpha = alphasDown_[static_cast<size_t>(n)];
      const float out = alpha * inOdd + lv1[n];
      lv1[n] = inOdd - alpha * out;
      inOdd = out;
    }

    // Mix with 0.5 gain and manage one-sample delay between paths
    outputData[i] = (delay + directOut) * 0.5f;
    delay = inOdd;
  }

  delayDown_ = delay;
}
}