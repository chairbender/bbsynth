#pragma once

namespace audio_plugin {

/**
 * A simple first-order high-pass filter for DC blocking.
 * y[n] = x[n] - x[n-1] + R * y[n-1]
 */
class DCBlocker {
 public:
  DCBlocker() = default;
  explicit DCBlocker(const float pole) : pole_{pole} {}

  /**
   * Processes a single sample through the DC blocker.
   */
  float Process(float sample);

  /**
   * Resets the internal state.
   */
  void Reset();

  /**
   * Sets the pole of the DC blocker (typically between 0.99 and 0.999).
   */
  void set_pole(const float pole) { pole_ = pole; }

 private:
  float pole_{0.995f};
  float x_prev_{0.0f};
  float y_prev_{0.0f};
};

}  // namespace audio_plugin
