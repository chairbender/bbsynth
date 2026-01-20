#include "DCBlocker.h"

#include "../Utils.h"

namespace audio_plugin {

float DCBlocker::Process(const float sample) {
  const float y = Sanitize(sample - x_prev_ + pole_ * y_prev_);
  x_prev_ = sample;
  y_prev_ = y;
  return y;
}

void DCBlocker::Reset() {
  x_prev_ = 0.0f;
  y_prev_ = 0.0f;
}

}  // namespace audio_plugin
