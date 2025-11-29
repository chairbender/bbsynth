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
      double pix = juce::MathConstants<double>::twoPi * x;
      return sin(pix) / pix;
    }
  }

  // Generate Blackman Window
  static inline void BlackmanWindow(size_t n, double* w) {
    const size_t m = n - 1;

    double fm = static_cast<double>(m);
    for (size_t i = 0; i <= m; i++) {
      const double f1 =
          (2.0 * juce::MathConstants<double>::twoPi * static_cast<double>(i)) /
          fm;
      const double f2 = 2.0 * f1;
      w[i] = 0.42 - (0.5 * cos(f1)) + (0.08 * cos(f2));
    }
  }

  // Discrete Fourier Transform
  static void DFT(const size_t n,
           const double* realTime,
           const double* imagTime,
           double* realFreq,
           double* imagFreq) {
    size_t k;

    for (k = 0; k < n; k++) {
      realFreq[k] = 0.0;
      imagFreq[k] = 0.0;
    }

    for (k = 0; k < n; k++)
      for (size_t i = 0; i < n; i++) {
        const double p = (2.0 * juce::MathConstants<double>::twoPi *
                    static_cast<double>(k * i)) /
                   static_cast<double>(n);
        const double sr = cos(p);
        const double si = -sin(p);
        realFreq[k] += (realTime[i] * sr) - (imagTime[i] * si);
        imagFreq[k] += (realTime[i] * si) + (imagTime[i] * sr);
      }
  }

  // Inverse Discrete Fourier Transform
  static void InverseDFT(const size_t n,
                  double* realTime,
                  double* imagTime,
                  const double* realFreq,
                  const double* imagFreq) {
    size_t k;

    for (k = 0; k < n; k++) {
      realTime[k] = 0.0;
      imagTime[k] = 0.0;
    }

    for (k = 0; k < n; k++) {
      for (size_t i = 0; i < n; i++) {
        const double p = (2.0 * juce::MathConstants<double>::twoPi *
                    static_cast<double>(k * i)) /
                   static_cast<double>(n);
        const double sr = cos(p);
        const double si = -sin(p);
        realTime[k] += (realFreq[i] * sr) + (imagFreq[i] * si);
        imagTime[k] += (realFreq[i] * si) - (imagFreq[i] * sr);
      }
      realTime[k] /= static_cast<double>(n);
      imagTime[k] /= static_cast<double>(n);
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

  // Compute Real Cepstrum Of Signal
  static void RealCepstrum(const size_t n, const double* signal, double* realCepstrum) {
    size_t i;

    const auto realTime = new double[n];
    const auto imagTime = new double[n];
    const auto realFreq = new double[n];
    const auto imagFreq = new double[n];

    // Compose Complex FFT Input
    for (i = 0; i < n; i++) {
      realTime[i] = signal[i];
      imagTime[i] = 0.0;
    }

    // Perform DFT

    DFT(n, realTime, imagTime, realFreq, imagFreq);

    // Calculate Log Of Absolute Value
    for (i = 0; i < n; i++) {
      realFreq[i] = log(cabs(realFreq[i], imagFreq[i]));
      imagFreq[i] = 0.0;
    }

    // Perform Inverse FFT
    InverseDFT(n, realTime, imagTime, realFreq, imagFreq);

    // Output Real Part Of FFT
    for (i = 0; i < n; i++)
      realCepstrum[i] = realTime[i];

    // todo not sure this is right - original was just delete
    delete[] realTime;
    delete[] imagTime;
    delete[] realFreq;
    delete[] imagFreq;
  }

  // Compute Minimum Phase Reconstruction Of Signal
  static void MinimumPhase(const size_t n,
                           const double* realCepstrum, double* minimumPhase) {
    size_t i;
    double *realFreq, *imagFreq;

    const size_t nd2 = n / 2;
    const auto realTime = new double[n];
    const auto imagTime = new double[n];
    realFreq = new double[n];
    imagFreq = new double[n];

    if ((n % 2) == 1) {
      realTime[0] = realCepstrum[0];
      for (i = 1; i < nd2; i++)
        realTime[i] = 2.0 * realCepstrum[i];
      for (i = nd2; i < n; i++)
        realTime[i] = 0.0;
    } else {
      realTime[0] = realCepstrum[0];
      for (i = 1; i < nd2; i++)
        realTime[i] = 2.0 * realCepstrum[i];
      realTime[nd2] = realCepstrum[nd2];
      for (i = nd2 + 1; i < n; i++)
        realTime[i] = 0.0;
    }

    for (i = 0; i < n; i++)
      imagTime[i] = 0.0;

    DFT(n, realTime, imagTime, realFreq, imagFreq);

    for (i = 0; i < n; i++)
      cexp(realFreq[i], imagFreq[i], &realFreq[i], &imagFreq[i]);

    InverseDFT(n, realTime, imagTime, realFreq, imagFreq);

    for (i = 0; i < n; i++)
      minimumPhase[i] = realTime[i];

    // todo correct to use brackets?
    delete[] realTime;
    delete[] imagTime;
    delete realFreq;
    delete imagFreq;
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
