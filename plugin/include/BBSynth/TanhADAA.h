#pragma once


namespace audio_plugin {
/**
 * 1-st order approximation of tanh function using ADAA
 */
class TanhADAA {
public:
  TanhADAA();

  float process(float x0);
  void reset();

private:
  // previous input
  float x1_;
};
}