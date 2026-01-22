#include "Downsampler.h"

#include <ranges>

namespace audio_plugin {

void Downsampler::prepare(const int max_block_size,
                          const int oversamplingFactor) {
  oversamplingFactor_ = oversamplingFactor;
  stages_.clear();

  int numStages = 0;
  int tempFactor = oversamplingFactor;
  while (tempFactor > 1) {
    if (tempFactor % 2 != 0) break;  // Only power of 2 supported
    tempFactor /= 2;
    numStages++;
  }

  if (numStages == 0) return;

  stages_.resize(static_cast<size_t>(numStages));

  // We design coefficients for each stage.
  // In JUCE's Oversampling, they use different coefficients for different
  // stages if it's multi-stage. For simplicity and matching their 2x polyphase
  // IIR:
  constexpr float twDown = 0.06f;
  constexpr float gaindBDown = -75.0f;

  auto structureDown = juce::dsp::FilterDesign<
      float>::designIIRLowpassHalfBandPolyphaseAllpassMethod(twDown,
                                                             gaindBDown);

  std::vector<float> alphas;
  alphas.reserve(structureDown.directPath.size() +
                 structureDown.delayedPath.size());
  for (const auto direct_path : structureDown.directPath)
    alphas.push_back(direct_path->coefficients[0]);
  for (const auto delayed_path : structureDown.delayedPath)
    alphas.push_back(delayed_path->coefficients[0]);

  for (auto& [stage_alphas, v1, delay] : std::views::take(stages_, numStages)) {
    stage_alphas = alphas;
    v1.assign(alphas.size(), 0.0f);
    delay = 0.0f;
  }

  if (numStages > 1) {
    // Internal buffer for intermediate stages
    // The largest intermediate buffer needed is for the first stage output
    internalBuffer_.setSize(1, max_block_size * (oversamplingFactor / 2));
  }
}

void Downsampler::process(const juce::AudioBuffer<float>& input,
                          juce::AudioBuffer<float>& output,
                          const int sourceStartSample,
                          const int sourceNumSamples,
                          const int destStartSample) {
  // todo: refactor pointless
  const int dest_start_sample = destStartSample;
  const int dest_num_samples = sourceNumSamples / oversamplingFactor_;
  if (stages_.empty()) {
    jassert(oversamplingFactor_ == 1);
    output.addFrom(0, sourceStartSample, input, 0, sourceStartSample,
                    sourceNumSamples);
    return;
  }

  const juce::AudioBuffer<float>* currentInput = &input;

  for (auto [s, stage] : std::views::enumerate(stages_)) {
    const int stageOutputSamples = dest_num_samples << (stages_.size() - 1 - s);
    const int stageStartSample = dest_start_sample << (stages_.size() - 1 - s);

    juce::AudioBuffer<float>* currentOutput;
    if (s == stages_.size() - 1) {
      currentOutput = &output;
    } else {
      currentOutput = &internalBuffer_;
    }

    auto* inputData = currentInput->getReadPointer(0);
    auto* outputData = currentOutput->getWritePointer(0);

    const auto numAlphas = static_cast<int>(stage.alphas.size());
    const int delayedStages = numAlphas / 2;
    const int directStages = numAlphas - delayedStages;
    const auto lv1 = std::span(stage.v1);
    float delay = stage.delay;

    if (stageStartSample <= stageOutputSamples) {
      for (const int i :
           std::views::iota(stageStartSample, stageOutputSamples)) {
        // Direct path cascaded allpass filters (even sample)
        float inEven = inputData[(i << 1)];
        for (auto [alpha, lv1_sample] : std::views::zip(stage.alphas, lv1) |
                                            std::views::take(directStages)) {
          const float out = alpha * inEven + lv1_sample;
          lv1_sample = inEven - alpha * out;
          inEven = out;
        }
        const float directOut = inEven;

        // Delayed path cascaded allpass filters (odd sample)
        float inOdd = inputData[(i << 1) + 1];
        for (auto [alpha, lv1_sample] : std::views::zip(stage.alphas, lv1) |
                                            std::views::drop(directStages) |
                                            std::views::take(delayedStages)) {
          const float out = alpha * inOdd + lv1_sample;
          lv1_sample = inOdd - alpha * out;
          inOdd = out;
        }

        // Mix with 0.5 gain and manage one-sample delay between paths
        outputData[i] += (delay + directOut) * 0.5f;
        delay = inOdd;
      }
    }
    stage.delay = delay;
    currentInput = currentOutput;
  }
}
}  // namespace audio_plugin