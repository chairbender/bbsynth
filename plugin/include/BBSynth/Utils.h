#pragma once
#include <algorithm>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {
// detects clipping within first channel of the buffer
static void DetectClip(const juce::AudioBuffer<float>& buffer, const std::string& label) {
  const auto data = buffer.getReadPointer(0);
  auto max = 0.f;
  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    max = std::max(max, std::abs(data[i]));
  }
  if (max > 1) {
    DBG(juce::String(label) + " clipping detected: " + juce::String(max));
  }
}
}
