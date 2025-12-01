/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/minBlepGenerator.h
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/

// todo rewrite to modern c++ standards
// todo cleanup / reduce need for static casting - some places are using size_t in places where the juce lib wants int.


#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace audio_plugin {

class MinBlepGenerator {
  // SEE ....
  // http://www.kvraudio.com/forum/viewtopic.php?t=364256
  // http://www.cs.cmu.edu/~eli/papers/icmc01-hardsync.pdf
  // http://stackoverflow.com/questions/175312/bandlimited-waveform-generation

  // Basically, we need an oversampled, filtered, nonlinearity .... 1->0 ...
  // This will be added any time the waveform jumps ....
  // in order to eliminate aliasing (basically, build a bandlimited wave)

  // ANTIALIASING FILTER ::::
  // since we are downsampling .... we can filter for better AA
  double coefficients[6];
  struct FilterState {
    double x1, x2, y1, y2;
  };
  int numChannels = 2;
  juce::HeapBlock<FilterState> filterStates;
  double ratio, lastRatio;

public:
  double overSamplingRatio;
  int zeroCrossings;

  float lastValue;
  float lastDelta;  // previous derivative ...

  // Tweaking the Blep F
  double proportionalBlepFreq;
  bool returnDerivative;  // set this to return the FIRST DERIVATIVE of the blep
                          // (for first der. discontinuities)

  struct BlepOffset {
    /**
     * This is a value representing a sample index (integer part) + subsample (fractional part).
     * But what is offset from is a little unintuitive...
     * Consider just the integer part for now:
     * When the blep occured in the current buffer, this will be a negative value
     * where the magnitude of the value matches the index
     * the blep occurred at.
     * It's set up so that as you walk through the current buffer (i + offset) = 0
     * when you've reached the sample where the blep happend, and gets more positive as you
     * continue to step through samples. This is used to convert to a lookup against the blep
     * table so we know what part of the blep table we should be mixing in for a given offset in
     * output samples from the start of the blep.
     * The sign flips to positive once we start processing the next buffer of audio, and
     * (todo presumably) the magnitude at that point represents how many samples ago the blep occurred
     * (so the blep tail is processed when it spans multiple buffers).
     */
    double offset = 0;
    double freqMultiple = 0;
    double pos_change_magnitude = 0;
    double vel_change_magnitude = 0;
  };

  juce::Array<BlepOffset, juce::CriticalSection> currentActiveBlepOffsets;

public:
  MinBlepGenerator();
  ~MinBlepGenerator();

  static juce::Array<float> getMinBlepArray();
  static juce::Array<float> getMinBlepDerivArray();

  void setToReturnDerivative(const bool derivative) { returnDerivative = derivative; }

  // TEMP
  // todo not implemented - delete?
  void setTest(double newTest);

  // Utility ....

  // SINC Function
  static inline double SINC(double x) {
    if (x == 0.0)
      return 1.0;
    else {
      double pix = juce::MathConstants<double>::pi * x;
      return sin(pix) / pix;
    }
  }

  // Generate Blackman Window
  static inline double blackmanHarris(double p) {
    return
      + 0.35875
      - 0.48829 * std::cos(2 * juce::MathConstants<double>::pi * p)
      + 0.14128 * std::cos(4 * juce::MathConstants<double>::pi * p)
      - 0.01168 * std::cos(6 * juce::MathConstants<double>::pi * p);
  }

  /**
   * Applies the window to x
   */
  static inline void ApplyBlackmanHarrisWindow(int n, double* x) {
    for (int i = 0; i < n; i++) {
      x[i] *= blackmanHarris(static_cast<double>(i) / (n - 1));
    }
  }

  // Discrete Fourier Transform
  static void DFT(const size_t N,
           const double* realTime,
           const double* imagTime,
           double* realFreq,
           double* imagFreq) {
    for (size_t k = 0; k < N; k++) {
      realFreq[k] = 0.0;
      imagFreq[k] = 0.0;
    }

    // Calculate DFT for each frequency bin k
    for (size_t k = 0; k < N; k++) {
      double realSum = 0.0;
      double imagSum = 0.0;

      // Sum over all input samples n
      for (size_t n = 0; n < N; n++) {
        double angle = -2.0 * M_PI * static_cast<double>(k)
          * static_cast<double>(n) / static_cast<double>(N);
        double cosAngle = cos(angle);
        double sinAngle = sin(angle);

        // Complex multiplication: (inputReal[n] + i*inputImag[n]) * (cos(angle) + i*sin(angle))
        realSum += realTime[n] * cosAngle - imagTime[n] * sinAngle;
        imagSum += realTime[n] * sinAngle + imagTime[n] * cosAngle;
      }

      realFreq[k] = realSum;
      imagFreq[k] = imagSum;
    }
  }

  // Inverse Discrete Fourier Transform.
  // Note the result is scaled by 1/n, which assumes the DFT was NOT scaled.
  static void InverseDFT(const size_t N,
                  double* realTime,
                  double* imagTime,
                  const double* realFreq,
                  const double* imagFreq) {
    for (size_t k = 0; k < N; k++) {
      realTime[k] = 0.0;
      imagTime[k] = 0.0;
    }

    // Calculate IDFT for each time sample n
    for (size_t n = 0; n < N; n++) {
      double realSum = 0.0;
      double imagSum = 0.0;

      // Sum over all frequency bins k
      for (size_t k = 0; k < N; k++) {
        double angle = 2.0 * M_PI * static_cast<double>(k)
          * static_cast<double>(n) / static_cast<double>(N);  // Note: positive angle (opposite of DFT)
        double cosAngle = cos(angle);
        double sinAngle = sin(angle);

        // Complex multiplication: (inputReal[k] + i*inputImag[k]) * (cos(angle) + i*sin(angle))
        realSum += realFreq[k] * cosAngle - imagFreq[k] * sinAngle;
        imagSum += realFreq[k] * sinAngle + imagFreq[k] * cosAngle;
      }

      // Normalize by dividing by N
      realTime[n] = realSum / static_cast<double>(N);
      imagTime[n] = imagSum / static_cast<double>(N);
    }
  }

  // Complex Absolute Value
  static inline double cabs(const double x, const double y) { return sqrt((x * x) + (y * y)); }

  // Complex Exponential
  static inline void cexp(const double x,
                          const double y, double* zx, double* zy) {
    const double expx = exp(x);
    *zx = expx * cos(y);
    *zy = expx * sin(y);
  }

  // Compute Real Cepstrum Of x
  static void RealCepstrum(const size_t n, double* x) {
    size_t i;

    const auto realTime = new double[n];
    const auto imagTime = new double[n];
    const auto realFreq = new double[n];
    const auto imagFreq = new double[n];

    // Compose Complex FFT Input
    for (i = 0; i < n; i++) {
      realTime[i] = x[i];
      imagTime[i] = 0.0;
    }

    DFT(n, realTime, imagTime, realFreq, imagFreq);

    // Note: For real cepstrum, we only return the real part
    // The imaginary part should be negligible (numerical errors only)
    for (i = 0; i < n; i++) {
      // Calculate magnitude: sqrt(real^2 + imag^2)
      double magnitude = sqrt(realFreq[i] * realFreq[i] +
                             imagFreq[i] * imagFreq[i]);

      // Take natural log (add small epsilon to avoid log(0))
      const double epsilon = 1e-10;
      realFreq[i] = log(magnitude + epsilon);
      imagFreq[i] = 0;
    }

    // Perform Inverse FFT (this also scales by 1/n)
    InverseDFT(n, realTime, imagTime, realFreq, imagFreq);

    // Output Real Part Of FFT
    for (i = 0; i < n; i++)
      x[i] = realTime[i];

    delete[] realTime;
    delete[] imagTime;
    delete[] realFreq;
    delete[] imagFreq;
  }

  // Compute Minimum Phase Reconstruction Of x
  static void MinimumPhase(const size_t n, double* x) {
    const auto realTime = new double[n];
    const auto imagTime = new double[n];
    const auto realFreq = new double[n];
    const auto imagFreq = new double[n];

    // Compose Complex FFT Input
    // keep DC component
    realTime[0] = x[0];
    for (size_t i = 1; i < n; i++) {
      realTime[i] = x[i];
      imagTime[i] = 0.0;
    }

    // double the positive freqs (causal part)
    for (size_t i = 0; i < n / 2; i++) {
      realTime[i] *= 2;
    }

    // nyquist freq (for even N)
    // todo assumes nyquist bin is half the input - is this correct?
    if (n % 2 == 0) {
      realTime[n / 2] = x[n / 2];
    }

    // zero out negative freqs (anti-causal part)
    for (size_t i = (n / 2) + 1; i < n; i++) {
      realTime[i] = 0;
    }

    DFT(n, realTime, imagTime, realFreq, imagFreq);

    // exponentiate to get complex spectrum
    for (size_t k = 0; k < n; k++) {
      double magnitude = exp(realFreq[k]);
      double phase = imagFreq[k];

      realFreq[k] = magnitude * cos(phase);
      imagFreq[k] = magnitude * sin(phase);
    }

    InverseDFT(n, realTime, imagTime, realFreq, imagFreq);

    for (size_t i = 0; i < n; i++)
      x[i] = realTime[i];

    delete[] realTime;
    delete[] imagTime;
    delete[] realFreq;
    delete[] imagFreq;
  }

  // FILTER ::::::
  void createLowPass(const double frequencyRatio) {
    const double proportionalRate =
        (frequencyRatio > 1.0) ? 0.5 / frequencyRatio : 0.5 * frequencyRatio;

    const double n = 1.0 / std::tan(juce::MathConstants<double>::twoPi *
                                    juce::jmax(0.001, proportionalRate));
    const double nSquared = n * n;
    const double c1 = 1.0 / (1.0 + std::sqrt(2.0) * n + nSquared);

    setFilterCoefficients(c1, c1 * 2.0, c1, 1.0, c1 * 2.0 * (1.0 - nSquared),
                          c1 * (1.0 - std::sqrt(2.0) * n + nSquared));
  }
  void setFilterCoefficients(double c1,
                             double c2,
                             double c3,
                             double c4,
                             double c5,
                             double c6) {
    const double a = 1.0 / c4;

    c1 *= a;
    c2 *= a;
    c3 *= a;
    c5 *= a;
    c6 *= a;

    coefficients[0] = c1;
    coefficients[1] = c2;
    coefficients[2] = c3;
    coefficients[3] = c4;
    coefficients[4] = c5;
    coefficients[5] = c6;
  }
  void resetFilters() { filterStates.clear(numChannels); }
  void applyFilter(float* samples, int num, FilterState& fs) const {
    while (--num >= 0) {
      const double in = static_cast<double>(*samples);

      double out = coefficients[0] * in + coefficients[1] * fs.x1 +
                   coefficients[2] * fs.x2 - coefficients[4] * fs.y1 -
                   coefficients[5] * fs.y2;

#if JUCE_INTEL
      if (!(out < -1.0e-8 || out > 1.0e-8))
        out = 0;
#endif

      fs.x2 = fs.x1;
      fs.x1 = in;
      fs.y2 = fs.y1;
      fs.y1 = out;

      *samples++ = static_cast<float>(out);
    }
  }
  float filterSample(float sample, FilterState& fs) const {
    const double in = static_cast<double>(sample);

    double out = coefficients[0] * in + coefficients[1] * fs.x1 +
                 coefficients[2] * fs.x2 - coefficients[4] * fs.y1 -
                 coefficients[5] * fs.y2;

#if JUCE_INTEL
    if (!(out < -1.0e-8 || out > 1.0e-8))
      out = 0;
#endif

    fs.x2 = fs.x1;
    fs.x1 = in;
    fs.y2 = fs.y1;
    fs.y1 = out;

    return static_cast<float>(out);
  }

  void clear();
  bool isClear() const;

  // CUSTOM ::::
  void setLimitingFreq(float proportionOfSamplingRate);

  void buildBlep();
  void addBlep(BlepOffset newBlep);
  void addBlepArray(const juce::Array<BlepOffset>& newBleps);

  juce::Array<BlepOffset> getNextBleps();

  void processBlock(float* buffer, int numSamples);
  void rescale_bleps_to_buffer(const float* buffer,
                               int numSamples,
                               float shiftBlepsBy = 0);
  void process_currentBleps(float* buffer, int numSamples);
};

}  // namespace audio_plugin
