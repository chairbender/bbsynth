#include "Downsampler.h"

namespace audio_plugin {

void Downsampler::Prepare(const int max_block_size) {
  downsampler_.reset();
  downsampler_.initProcessing (static_cast<size_t>(max_block_size));

}
// TODO: not sure if better to pass by const reference, not sure about const float, etc...
//   just trying to get it working first...
inline juce::dsp::AudioBlock<const float> Downsampler::GetOverSampleBuffer(
    const juce::dsp::ProcessContextReplacing<float> &context) {
  return downsampler_.getUnprocessedUpsampleBlock (context.getInputBlock());
}
void Downsampler::Downsample(
    const juce::dsp::ProcessContextReplacing<float> &context) {
  downsampler_.processSamplesDown(context.getOutputBlock());
}

}  // namespace audio_plugin