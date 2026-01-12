#pragma once
import JuceImports;
import std;

namespace audio_plugin {
/**
 * the "tone" knob, which is basically a "tilt EQ" that
 * controls a low / high shelf via the "tilt" value (-1 to 1).
 * Only processes the first channel (mono)
 */
class ToneFilter {
 public:
  /**
   * Must be called prior to processing
   */
  void Prepare(double sample_rate, int block_size);
  void set_tilt(float tilt);
  /**
   * Only processes the first channel.
   */
  void Process(juce::AudioBuffer<float>& buffers, int numSamples);

 private:
  // -1 to 1 to control low / high shelf respectively
  float tilt_{0};
  double sample_rate_{0};
  juce::dsp::IIR::Filter<float> low_shelf_;
  juce::dsp::IIR::Filter<float> high_shelf_;
};
}  // namespace audio_plugin
