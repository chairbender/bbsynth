
#include "BBSynth/TanhADAA.h"

#include <cmath>

namespace audio_plugin {

TanhADAA::TanhADAA() : x1_(0.0f) {}

float TanhADAA::process(const float x0) {
  float y;

  const float tol = 1e-3f;

  if (const auto dx = x0 - x1_; std::fabs(dx) < tol) {
    // When inputs are too close, use Taylor series
    // to avoid numerical issues
    const auto xbar = 0.5f * (x0 + x1_);
    const auto sech2 = 1.0f - std::tanh(xbar) * std::tanh(xbar);
    y = std::tanh(xbar) + (dx / 2.0f) * sech2;
  } else {
    // Use the antiderivative formula
    // AD[tanh(x)] = log(cosh(x))
    y = (std::logf(std::coshf(x0)) - std::logf(std::coshf(x1_))) / dx;
  }

  x1_ = x0;
  return y;
}

void TanhADAA::reset() {
  x1_ = 0.0f;
}

}