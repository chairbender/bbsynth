
#include "BBSynth/TanhADAA.h"

#include <cmath>

namespace audio_plugin {

TanhADAA::TanhADAA() : x1_(0.0f) {}


// To avoid overflow with log(cosh(x)) for large x, we use the identity:
// log(cosh(x)) = |x| - log(2) + log(1 + exp(-2|x|))
// For large |x|, this is approximately |x| - log(2).
inline float LogCosh(const float x) {
  const float ax = std::fabs(x);
  if (ax > 20.0f) {
    return ax - std::log(2.0f);
  }
  return std::log(std::cosh(ax));
}

// todo i think this impl still isn't correct - giving nans/inf when oversampling is only 1x
float TanhADAA::process(const float x0) {
  float y;

  const float tol = 1e-3f;

  if (const auto dx = x0 - x1_; std::fabs(dx) < tol) {
    const auto xbar = 0.5f * (x0 + x1_);
    const auto tanhVal = std::tanh(xbar);
    const auto sech2 = 1.0f - tanhVal * tanhVal;

    // First-order Taylor approx.
    y = tanhVal + (dx / 2.0f) * sech2;
  } else {
    // Use the antiderivative formula
    // AD[tanh(x)] = log(cosh(x))
    y = (LogCosh(x0) - LogCosh(x1_)) / dx;
  }

  x1_ = x0;
  return y;
}

void TanhADAA::reset() {
  x1_ = 0.0f;
}

}