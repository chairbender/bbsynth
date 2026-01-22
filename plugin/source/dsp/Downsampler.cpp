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
  const int dest_num_samples = sourceNumSamples / oversamplingFactor_;
  if (stages_.empty()) {
    jassert(oversamplingFactor_ == 1);
    output.addFrom(0, destStartSample, input, 0, sourceStartSample,
                   sourceNumSamples);
    return;
  }

  const juce::AudioBuffer<float>* current_input = &input;
  int current_source_start_sample = sourceStartSample;

  for (auto [s, stage] : std::views::enumerate(stages_)) {
    const int stage_output_samples = dest_num_samples << (stages_.size() - 1 - s);
    const int stage_dest_start_sample =
        destStartSample << (stages_.size() - 1 - s);

    juce::AudioBuffer<float>* current_output;
    if (s == static_cast<int>(stages_.size() - 1)) {
      current_output = &output;
    } else {
      current_output = &internalBuffer_;
    }

    auto* input_data = current_input->getReadPointer(0);
    auto* output_data = current_output->getWritePointer(0);

    const auto num_alphas = static_cast<int>(stage.alphas.size());
    const int delayed_stages = num_alphas / 2;
    const int direct_stages = num_alphas - delayed_stages;
    const auto lv1 = std::span(stage.v1);
    float delay = stage.delay;

    for (const int i : std::views::iota(0, stage_output_samples)) {
      const int read_idx = (current_source_start_sample + i) << 1;
      const int write_idx = stage_dest_start_sample + i;

      // Direct path cascaded allpass filters (even sample)
      float in_even = input_data[read_idx];
      for (auto [alpha, lv1_sample] : std::views::zip(stage.alphas, lv1) |
                                          std::views::take(direct_stages)) {
        const float out = alpha * in_even + lv1_sample;
        lv1_sample = in_even - alpha * out;
        in_even = out;
      }
      const float direct_out = in_even;

      // Delayed path cascaded allpass filters (odd sample)
      float in_odd = input_data[read_idx + 1];
      for (auto [alpha, lv1_sample] : std::views::zip(stage.alphas, lv1) |
                                          std::views::drop(direct_stages) |
                                          std::views::take(delayed_stages)) {
        const float out = alpha * in_odd + lv1_sample;
        lv1_sample = in_odd - alpha * out;
        in_odd = out;
      }

      // Mix with 0.5 gain and manage one-sample delay between paths
      output_data[write_idx] += (delay + direct_out) * 0.5f;
      delay = in_odd;
    }

    stage.delay = delay;
    current_input = current_output;
    current_source_start_sample = stage_dest_start_sample;
  }
}
}  // namespace audio_plugin