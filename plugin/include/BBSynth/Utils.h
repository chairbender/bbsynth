#pragma once
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

/**
 * Sanitizes a float value by checking for INF and NAN.
 * Logs an error to DBG if detected.
 * Returns 1.0f for INF and 0.0f for NAN.
 */
inline float Sanitize(float value) {
  if (std::isinf(value)) {
    DBG("INF detected! Setting to 1.0f");
    return 1.0f;
  }
  if (std::isnan(value)) {
    DBG("NAN detected! Setting to 0.0f");
    return 0.0f;
  }
  return value;
}

// detects clipping within first channel of the buffer
// todo: use templating to have this in place but turn it off when not debugging
// static void DetectClip(const juce::AudioBuffer<float>& buffer, const std::string& label) {
//   const auto data = buffer.getReadPointer(0);
//   auto max = 0.f;
//   for (int i = 0; i < buffer.getNumSamples(); ++i) {
//     max = std::max(max, std::abs(data[i]));
//   }
//   if (max > 1) {
//     DBG(juce::String(label) + " clipping detected: " + juce::String(max));
//   }
// }
}
