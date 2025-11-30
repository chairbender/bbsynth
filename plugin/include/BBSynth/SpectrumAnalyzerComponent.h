#pragma once

#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

/**
 * A custom component for displaying a spectrum.
 * Based on AudioVisualizerComponent.
 * Only the left channel is visualized.
 */
class SpectrumAnalyzerComponent : public juce::Component, juce::Timer {
public:
  SpectrumAnalyzerComponent();
  /** Destructor. */
  ~SpectrumAnalyzerComponent() override;

  /**
   * must be called by root component and pass the current buffer
   */
  void getNextAudioBlock(const juce::AudioBuffer<float>& buffer);

  //==============================================================================
  /** @internal */
  void paint(juce::Graphics&) override;

    // todo - very weird syntax, what is this? why not constexpr?
  enum {
    fftOrder  = 11,             // [1]
    fftSize   = 1 << fftOrder,  // [2]
    scopeSize = 512             // [3]
  };
private:
  void timerCallback() override;
  void pushNextSampleIntoFifo(float sample) noexcept;
  void drawNextFrameOfSpectrum();
  void drawFrame(juce::Graphics& g);

  juce::dsp::FFT forwardFFT;                   // [4]
  juce::dsp::WindowingFunction<float> window;  // [5]

  // todo use proper c++ types
  float fifo[fftSize];             // [6]
  float fftData[2 * fftSize];      // [7]
  int fifoIndex = 0;               // [8]
  bool nextFFTBlockReady = false;  // [9]
  float scopeData[scopeSize];      // [10]

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerComponent)
};
}  // namespace audio_plugin
