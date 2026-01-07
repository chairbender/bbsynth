#include "ToneFilter.h"

namespace audio_plugin {

void ToneFilter::Prepare(const double sample_rate, const int block_size) {
  sample_rate_ = sample_rate;
  low_shelf_.prepare(juce::dsp::ProcessSpec(
      sample_rate, static_cast<juce::uint32>(block_size), 1));
  high_shelf_.prepare(juce::dsp::ProcessSpec(
      sample_rate, static_cast<juce::uint32>(block_size), 1));
  low_shelf_.reset();
  high_shelf_.reset();
}

void ToneFilter::set_tilt(const float tilt) {
  tilt_ = tilt;

  const float gain_db = tilt * 12.f;

  // todo : make some of these constants controllable so we can experiment
  //   with what works best for tone
  low_shelf_.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
      sample_rate_, 200.0f, 0.707f,
      juce::Decibels::decibelsToGain(-gain_db)  // Inverse gain for tilt
  );

  high_shelf_.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      sample_rate_, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(gain_db));
}

void ToneFilter::Process(juce::AudioBuffer<float>& buffers, int numSamples) {
  auto block = juce::dsp::AudioBlock<float>{buffers.getArrayOfWritePointers(),
  1, static_cast<size_t>(numSamples)};
  const auto process = juce::dsp::ProcessContextReplacing<float>(block);
  low_shelf_.process(process);
  high_shelf_.process(process);
}

}  // namespace audio_plugin