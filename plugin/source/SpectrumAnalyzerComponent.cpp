#include "BBSynth/SpectrumAnalyzerComponent.h"

namespace audio_plugin {

struct SpectrumAnalyzerComponent::ChannelInfo {
  ChannelInfo(SpectrumAnalyzerComponent& o, int bufferSize) : owner(o) {
    setBufferSize(bufferSize);
    clear();
  }

  void clear() noexcept {
    levels.fill({});
    value = {};
    subSample = 0;
  }

  void pushSamples(const float* inputSamples, int num) noexcept {
    for (int i = 0; i < num; ++i)
      pushSample(inputSamples[i]);
  }

  void pushSample(float newSample) noexcept {
    if (--subSample <= 0) {
      if (++nextSample == levels.size())
        nextSample = 0;

      levels.getReference(nextSample) = value;
      subSample = owner.getSamplesPerBlock();
      value = juce::Range<float>(newSample, newSample);
    } else {
      value = value.getUnionWith(newSample);
    }
  }

  void setBufferSize(int newSize) {
    levels.removeRange(newSize, levels.size());
    levels.insertMultiple(-1, {}, newSize - levels.size());

    if (nextSample >= newSize)
      nextSample = 0;
  }

  SpectrumAnalyzerComponent& owner;
  juce::Array<juce::Range<float>> levels;
  juce::Range<float> value;
  std::atomic<int> nextSample{0}, subSample{0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelInfo)
};

SpectrumAnalyzerComponent::SpectrumAnalyzerComponent(int initialNumChannels)
    : numSamples(1024),
      inputSamplesPerBlock(256),
      backgroundColour(juce::Colours::black),
      waveformColour(juce::Colours::white),
      forwardFFT{fftOrder},
      window{fftSize, juce::dsp::WindowingFunction<float>::hann} {
  setOpaque(true);
  setNumChannels(initialNumChannels);
  setRepaintRate(60);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent() {}

void SpectrumAnalyzerComponent::setNumChannels(int numChannels) {
  channels.clear();

  for (int i = 0; i < numChannels; ++i)
    channels.add(new ChannelInfo(*this, numSamples));
}

void SpectrumAnalyzerComponent::setBufferSize(int newNumSamples) {
  numSamples = newNumSamples;

  for (auto* c : channels)
    c->setBufferSize(newNumSamples);
}

void SpectrumAnalyzerComponent::clear() {
  for (auto* c : channels)
    c->clear();
}

void SpectrumAnalyzerComponent::pushBuffer(const float* const* d,
                                           int numChannels,
                                           int num) {
  numChannels = juce::jmin(numChannels, channels.size());

  for (int i = 0; i < numChannels; ++i)
    channels.getUnchecked(i)->pushSamples(d[i], num);
}

void SpectrumAnalyzerComponent::pushBuffer(
    const juce::AudioBuffer<float>& buffer) {
  pushBuffer(buffer.getArrayOfReadPointers(), buffer.getNumChannels(),
             buffer.getNumSamples());
}

void SpectrumAnalyzerComponent::pushBuffer(
    const juce::AudioSourceChannelInfo& buffer) {
  auto numChannels =
      juce::jmin(buffer.buffer->getNumChannels(), channels.size());

  for (int i = 0; i < numChannels; ++i)
    channels.getUnchecked(i)->pushSamples(
        buffer.buffer->getReadPointer(i, buffer.startSample),
        buffer.numSamples);
}

void SpectrumAnalyzerComponent::pushSample(const float* d, int numChannels) {
  numChannels = juce::jmin(numChannels, channels.size());

  for (int i = 0; i < numChannels; ++i)
    channels.getUnchecked(i)->pushSample(d[i]);
}

void SpectrumAnalyzerComponent::setSamplesPerBlock(
    int newSamplesPerPixel) noexcept {
  inputSamplesPerBlock = newSamplesPerPixel;
}

void SpectrumAnalyzerComponent::setRepaintRate(int frequencyInHz) {
  startTimerHz(frequencyInHz);
}

void SpectrumAnalyzerComponent::timerCallback() {
  repaint();
}

void SpectrumAnalyzerComponent::setColours(juce::Colour bk,
                                           juce::Colour fg) noexcept {
  backgroundColour = bk;
  waveformColour = fg;
  repaint();
}

void SpectrumAnalyzerComponent::paint(juce::Graphics& g) {
  g.fillAll(backgroundColour);

  auto r = getLocalBounds().toFloat();
  auto channelHeight = r.getHeight() / static_cast<float>(channels.size());

  g.setColour(waveformColour);

  for (auto* c : channels)
    paintChannel(g, r.removeFromTop(channelHeight), c->levels.begin(),
                 c->levels.size(), c->nextSample);
}

void SpectrumAnalyzerComponent::getChannelAsPath(
    juce::Path& path,
    const juce::Range<float>* levels,
    int numLevels,
    int nextSample) {
  path.preallocateSpace(4 * numLevels + 8);

  for (int i = 0; i < numLevels; ++i) {
    auto level = -(levels[(nextSample + i) % numLevels].getEnd());

    if (i == 0)
      path.startNewSubPath(0.0f, level);
    else
      path.lineTo(static_cast<float>(i), level);
  }

  for (int i = numLevels; --i >= 0;)
    path.lineTo(static_cast<float>(i), -(levels[(nextSample + i) % numLevels].getStart()));

  path.closeSubPath();
}

void SpectrumAnalyzerComponent::paintChannel(juce::Graphics& g,
                                             juce::Rectangle<float> area,
                                             const juce::Range<float>* levels,
                                             int numLevels,
                                             int nextSample) {
  juce::Path p;
  getChannelAsPath(p, levels, numLevels, nextSample);

  g.fillPath(p, juce::AffineTransform::fromTargetPoints(
                    0.0f, -1.0f, area.getX(), area.getY(), 0.0f, 1.0f,
                    area.getX(), area.getBottom(), static_cast<float>(numLevels), -1.0f,
                    area.getRight(), area.getY()));
}
}  // namespace audio_plugin