#include "SpectrumAnalyzerComponent.h"

#include <ranges>
#include <span>

namespace audio_plugin {

SpectrumAnalyzerComponent::SpectrumAnalyzerComponent()
    : forwardFFT{fftOrder},
      window{fftSize, juce::dsp::WindowingFunction<float>::hann} {
  setOpaque(true);
  startTimerHz(30);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent() {}

void SpectrumAnalyzerComponent::getNextAudioBlock(
    const juce::AudioBuffer<float>& buffer) {
  if (buffer.getNumChannels() > 0) {
    // todo is 0 correct for sampleidx?
    const auto channel_data_span =
        std::span{buffer.getReadPointer(0, 0), static_cast<size_t>(buffer.getNumSamples())};

    for (const auto sample : channel_data_span) pushNextSampleIntoFifo(sample);
  }
}

//==============================================================================
void SpectrumAnalyzerComponent::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);

  g.setOpacity(1.0f);
  g.setColour(juce::Colours::white);
  drawFrame(g);
}

void SpectrumAnalyzerComponent::timerCallback() {
  if (nextFFTBlockReady) {
    drawNextFrameOfSpectrum();
    nextFFTBlockReady = false;
    repaint();
  }
}

void SpectrumAnalyzerComponent::pushNextSampleIntoFifo(float sample) noexcept {
  // if the fifo contains enough data, set a flag to say
  // that the next frame should now be rendered..
  if (fifoIndex == fftSize)  // [11]
  {
    if (!nextFFTBlockReady)  // [12]
    {
      juce::zeromem(fftData, sizeof(fftData));
      memcpy(fftData, fifo, sizeof(fifo));
      nextFFTBlockReady = true;
    }

    fifoIndex = 0;
  }

  fifo[fifoIndex++] = sample;  // [12]
}

void SpectrumAnalyzerComponent::drawNextFrameOfSpectrum() {
  // first apply a windowing function to our data
  window.multiplyWithWindowingTable(fftData, fftSize);  // [1]

  // then render our FFT data..
  forwardFFT.performFrequencyOnlyForwardTransform(fftData);  // [2]

  for (const auto [i, scope_sample] : std::views::enumerate(scopeData))  // [3]
  {
    constexpr auto mindB = -100.0f;
    constexpr auto maxdB = 0.0f;

    auto skewedProportionX =
        1.0f - std::exp(std::log(1.0f - static_cast<float>(i) / static_cast<float>(scopeSize)) * 0.2f);
    auto fftDataIndex = juce::jlimit(
        0, fftSize / 2, static_cast<int>(skewedProportionX * static_cast<float>(fftSize) * 0.5f));
    auto level = juce::jmap(
        juce::jlimit(mindB, maxdB,
                     juce::Decibels::gainToDecibels(fftData[fftDataIndex]) -
                         juce::Decibels::gainToDecibels(static_cast<float>(fftSize))),
        mindB, maxdB, 0.0f, 1.0f);

    scope_sample = level;  // [4]
  }
}

void SpectrumAnalyzerComponent::drawFrame(juce::Graphics& g) const {
  for (const int i : std::views::iota(1, scopeSize)) {
    const auto width = getLocalBounds().getWidth();
    const auto height = getLocalBounds().getHeight();

    g.drawLine({static_cast<float>(juce::jmap(i - 1, 0, scopeSize - 1, 0, width)),
                juce::jmap(scopeData[i - 1], 0.0f, 1.0f, static_cast<float>(height), 0.0f),
                static_cast<float>(juce::jmap(i, 0, scopeSize - 1, 0, width)),
                juce::jmap(scopeData[i], 0.0f, 1.0f, static_cast<float>(height), 0.0f)});
  }
}
}  // namespace audio_plugin