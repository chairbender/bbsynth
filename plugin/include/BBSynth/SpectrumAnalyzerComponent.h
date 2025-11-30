#pragma once

#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

/**
 * A custom component for displaying a spectrum.
 * Based on AudioVisualizerComponent.
 */
class SpectrumAnalyzerComponent : public juce::Component, juce::Timer {
public:
  SpectrumAnalyzerComponent(int initialNumChannels);
  /** Destructor. */
  ~SpectrumAnalyzerComponent() override;

  /** Changes the number of channels that the visualiser stores. */
  void setNumChannels(int numChannels);

  /** Changes the number of samples that the visualiser keeps in its history.
      Note that this value refers to the number of averaged sample blocks, and
     each block is calculated as the peak of a number of incoming audio samples.
     To set the number of incoming samples per block, use setSamplesPerBlock().
   */
  void setBufferSize(int bufferSize);

  /** */
  void setSamplesPerBlock(int newNumInputSamplesPerBlock) noexcept;

  /** */
  int getSamplesPerBlock() const noexcept { return inputSamplesPerBlock; }

  /** Clears the contents of the buffers. */
  void clear();

  /** Pushes a buffer of channels data.
      The number of channels provided here is expected to match the number of
     channels that this AudioVisualiserComponent has been told to use.
  */
  void pushBuffer(const juce::AudioBuffer<float>& bufferToPush);

  /** Pushes a buffer of channels data.
      The number of channels provided here is expected to match the number of
     channels that this AudioVisualiserComponent has been told to use.
  */
  void pushBuffer(const juce::AudioSourceChannelInfo& bufferToPush);

  /** Pushes a buffer of channels data.
      The number of channels provided here is expected to match the number of
     channels that this AudioVisualiserComponent has been told to use.
  */
  void pushBuffer(const float* const* channelData,
                  int numChannels,
                  int numSamples);

  /** Pushes a single sample (per channel).
      The number of channels provided here is expected to match the number of
     channels that this AudioVisualiserComponent has been told to use.
  */
  void pushSample(const float* samplesForEachChannel, int numChannels);

  /** Sets the colours used to paint the */
  void setColours(juce::Colour backgroundColour, juce::Colour waveformColour) noexcept;

  /** Sets the frequency at which the component repaints itself. */
  void setRepaintRate(int frequencyInHz);

  /** Draws a channel of audio data in the given bounds.
      The default implementation just calls getChannelAsPath() and fits this
     into the given area. You may want to override this to draw things
     differently.
  */
  virtual void paintChannel(juce::Graphics&,
                            juce::Rectangle<float> bounds,
                            const juce::Range<float>* levels,
                            int numLevels,
                            int nextSample);

  /** Creates a path which contains the waveform shape of a given set of range
     data. The path is normalised so that -1 and +1 are its upper and lower
     bounds, and it goes from 0 to numLevels on the X axis.
  */
  void getChannelAsPath(juce::Path& result,
                        const juce::Range<float>* levels,
                        int numLevels,
                        int nextSample);

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
  struct ChannelInfo;

  juce::OwnedArray<ChannelInfo> channels;
  int numSamples, inputSamplesPerBlock;
  juce::Colour backgroundColour, waveformColour;

  void timerCallback() override;

  juce::dsp::FFT forwardFFT;                   // [4]
  juce::dsp::WindowingFunction<float> window;  // [5]

  // todo relocate vars properly between component vs. editor
  // todo use proper c++ types
  // todo refactor to separate component
  float fifo[fftSize];             // [6]
  float fftData[2 * fftSize];      // [7]
  int fifoIndex = 0;               // [8]
  bool nextFFTBlockReady = false;  // [9]
  float scopeData[scopeSize];      // [10]

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerComponent)
};
}  // namespace audio_plugin
