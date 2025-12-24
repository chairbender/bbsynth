
#include "BBSynth/TanhADAA.h"

#include <cmath>

namespace audio_plugin {

TanhADAA::TanhADAA() : x1_(0.0f) {}

float TanhADAA::process(const float x0) {
  float y;

  const float tol = 1e-3f;

  // TODO: Not sure this is really needed anymore...
  if (const auto dx = x0 - x1_; std::fabs(dx) < tol) {
    const auto xbar = 0.5f * (x0 + x1_);
    const auto tanhVal = std::tanh(xbar);
    const auto sech2 = 1.0f - tanhVal * tanhVal;

    // First-order Taylor: good
    y = tanhVal + (dx / 2.0f) * sech2;

    // Or second-order for better accuracy:
     //const auto sech2_deriv = -2.0f * tanhVal * sech2;
     //y = tanhVal + (dx / 2.0f) * sech2 + (dx * dx / 24.0f) * sech2_deriv;
  } else {
    // Use the antiderivative formula
    // AD[tanh(x)] = log(cosh(x))
    y = (std::log(std::cosh(std::fabs(x0))) - std::log(std::cosh(std::fabs(x1_)))) / dx;
  }

  x1_ = x0;
  return y;
}

void TanhADAA::reset() {
  x1_ = 0.0f;
}

}