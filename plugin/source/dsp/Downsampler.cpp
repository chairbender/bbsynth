#include "Downsampler.h"

namespace audio_plugin {

void Downsampler::Prepare(const int max_block_size) {
  downsampler_.reset();
  downsampler_.initProcessing (static_cast<size_t>(max_block_size));

}

juce::dsp::AudioBlock<float> Downsampler::GetOverSampleBuffer(
    const juce::dsp::AudioBlock<float>& inputBlock) {
  return downsampler_.getUnprocessedUpsampleBlock(inputBlock);
}

void Downsampler::Downsample(juce::dsp::AudioBlock<float>& inputBlock) {
  downsampler_.processSamplesDown(inputBlock);
}

}  // namespace audio_plugin