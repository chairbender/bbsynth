
#include "BBSynth/TanhADAA.h"

#include <algorithm>
#include <cmath>

#include "juce_core/system/juce_PlatformDefs.h"

namespace audio_plugin {

TanhADAA::TanhADAA() : x1_(0.0f) {}

// To avoid overflow with log(cosh(x)) for large x, we use the identity:
// log(cosh(x)) = |x| - log(2) + log(1 + exp(-2|x|))
// For large |x|, this is approximately |x| - log(2).
inline float LogCosh(const float x) {
  const float ax = std::fabs(x);
  if (ax > 40.0f) {
    return ax - std::log(2.0f);
  }
  return std::log(std::cosh(ax));
}

float TanhADAA::process(const float x0) {
  float y;

  const float dx = x0 - x1_;
  const float absDx = std::fabs(dx);

  // Case 1: Very small step - use Taylor
  if (absDx < 1e-3f) {
    const float xbar = 0.5f * (x0 + x1_);
    const float tanhVal = std::tanh(xbar);
    const float sech2 = 1.0f - tanhVal * tanhVal;
    y = tanhVal + (dx / 2.0f) * sech2;
  }
  // Case 2: Saturated region - use midpoint tanh
  else if (std::fabs(x0) > 8.0f && std::fabs(x1_) > 8.0f) {
    const float xbar = 0.5f * (x0 + x1_);
    y = std::tanh(xbar);
  }
  // Case 3: Normal - use antiderivative
  else {
    y = (LogCosh(x0) - LogCosh(x1_)) / dx;
  }

  x1_ = x0;

  // Theoretical bounds enforcement (should rarely trigger)
  if (std::fabs(y) > 1.0f) {
    return std::clamp(y, -1.0f, 1.0f);
  }

  return y;
}

void TanhADAA::reset() { x1_ = 0.0f; }

}  // namespace audio_plugin