/*
Originally taken from (and then modified)
https://github.com/aaronleese/JucePlugin-Synth-with-AntiAliasing/blob/master/Source/minBlepGenerator.h
Used with permission:
https://forum.juce.com/t/open-source-square-waves-for-the-juceplugin/19915/8
*/

// todo rewrite to modern c++ standards
// todo cleanup / reduce need for static casting - some places are using size_t
// in places where the juce lib wants int.

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <array>
#include <ranges>

#include "../Constants.h"

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
  double coefficients_[6];
  struct FilterState {
    double x1_, x2_, y1_, y2_;
  };
  int num_channels_ = 2;
  juce::HeapBlock<FilterState> filter_states_;
  double ratio_{0.0}, last_ratio_{0.0};

  static constexpr int kRingBufferSize{kOversample * kBlepTableSize * 2};
  /** ring buffer containing the current blep signals to apply
   * readIndex indicates the position in the ring_buffer_ that aligns with
   * the current output buffer index 0.
   * **/
  std::array<float, kRingBufferSize> ring_buffer_{};
  int read_index_{0};

 public:

  float last_value_;
  float last_delta_;  // previous derivative ...

  // Tweaking the Blep F
  double proportional_blep_freq_;
  bool return_derivative_;  // set this to return the FIRST DERIVATIVE of the
                            // blep (for first der. discontinuities)
  // when true, aa responds to proportional blep freq.
  // when false, aa is fixed at nyquist.
  // generally, aa scaling may sound more "analog" as it mimicks the limitations
  // of the original analog oscillators (not able to perfectly output
  // all partials of the oscillator due to factors like slew rate limits in op amps,
  // parasitic capacitance, and component bandwidth limitations).
  // But, it's provided as a toggle so we can see for ourselves if that's true!
  bool aa_scaling_;

  struct BlepOffset {
    // index in current buffer where the blep starts
    double offset = 0;
    double pos_change_magnitude = 0;
    double vel_change_magnitude = 0;
  };

  MinBlepGenerator();
  ~MinBlepGenerator();

  static juce::Array<float> min_blep_array();
  static juce::Array<float> min_blep_deriv_array();

  void set_return_derivative(const bool derivative) {
    return_derivative_ = derivative;
  }

  // Utility ....

  // SINC Function
  static double Sinc(const double x) {
    if (x == 0.0)
      return 1.0;
    else {
      double pix = juce::MathConstants<double>::pi * x;
      return sin(pix) / pix;
    }
  }

  // Generate Blackman Window
  static double BlackmanHarris(const double p) {
    return +0.35875 -
           0.48829 * std::cos(2 * juce::MathConstants<double>::pi * p) +
           0.14128 * std::cos(4 * juce::MathConstants<double>::pi * p) -
           0.01168 * std::cos(6 * juce::MathConstants<double>::pi * p);
  }

  /**
   * Applies the window to x
   */
  static void ApplyBlackmanHarrisWindow(std::span<double> x) {
    for (auto [i, sample] : std::views::enumerate(x)) {
      sample *= BlackmanHarris(static_cast<double>(i) /
                               (static_cast<double>(x.size()) - 1));
    }
  }

  // Discrete Fourier Transform
  static void DFT(const std::span<const double> realTime,
                  const std::span<const double> imagTime, const std::span<double> realFreq,
                  const std::span<double> imagFreq) {
    std::ranges::fill(realFreq, 0.0);
    std::ranges::fill(imagFreq, 0.0);

    // Calculate DFT for each frequency bin k
    for (auto [k, freq_sample] :
         std::views::enumerate(std::views::zip(realFreq, imagFreq))) {
      auto& [real_freq_sample, imag_freq_sample] = freq_sample;
      double realSum = 0.0;
      double imagSum = 0.0;

      // Sum over all input samples n
      for (auto [n, time_sample] :
           std::views::enumerate(std::views::zip(realTime, imagTime))) {
        auto& [real_time_sample, imag_time_sample] = time_sample;
        const double angle = -2.0 * juce::MathConstants<double>::pi *
                             static_cast<double>(k) * static_cast<double>(n) /
                             static_cast<double>(realTime.size());
        const double cosAngle = cos(angle);
        const double sinAngle = sin(angle);

        // Complex multiplication: (inputReal[n] + i*inputImag[n]) * (cos(angle)
        // + i*sin(angle))
        realSum += real_time_sample * cosAngle - imag_time_sample * sinAngle;
        imagSum += imag_time_sample * sinAngle + imag_time_sample * cosAngle;
      }

      real_freq_sample = realSum;
      imag_freq_sample = imagSum;
    }
  }

  // Inverse Discrete Fourier Transform.
  // Note the result is scaled by 1/n, which assumes the DFT was NOT scaled.
  static void InverseDFT(const std::span<double> realTime,
                         const std::span<double> imagTime,
                         const std::span<const double> realFreq,
                         const std::span<const double> imagFreq) {
    std::ranges::fill(realTime, 0.0);
    std::ranges::fill(imagTime, 0.0);

    const auto n_samples = static_cast<double>(realTime.size());

    // Calculate IDFT for each time sample n
    for (auto [n, time_sample] :
         std::views::enumerate(std::views::zip(realTime, imagTime))) {
      auto& [real_time_sample, imag_time_sample] = time_sample;
      double real_sum = 0.0;
      double imag_sum = 0.0;

      // Sum over all frequency bins k
      for (auto [k, freq_sample] :
           std::views::enumerate(std::views::zip(realFreq, imagFreq))) {
        auto& [real_freq_sample, imag_freq_sample] = freq_sample;
        const double angle = 2.0 * juce::MathConstants<double>::pi *
                             static_cast<double>(k) * static_cast<double>(n) /
                             n_samples;  // Note: positive angle (opposite of DFT)
        const double cos_angle = cos(angle);
        const double sin_angle = sin(angle);

        // Complex multiplication: (inputReal[k] + i*inputImag[k]) * (cos(angle)
        // + i*sin(angle))
        real_sum += real_freq_sample * cos_angle - imag_freq_sample * sin_angle;
        imag_sum += real_freq_sample * sin_angle + imag_freq_sample * cos_angle;
      }

      // Normalize by dividing by N
      real_time_sample = real_sum / n_samples;
      imag_time_sample = imag_sum / n_samples;
    }
  }

  // Complex Absolute Value
  static inline double Cabs(const double x, const double y) {
    return sqrt((x * x) + (y * y));
  }

  // Complex Exponential
  static inline void Cexp(const double x, const double y, double* zx,
                          double* zy) {
    const double expx = exp(x);
    *zx = expx * cos(y);
    *zy = expx * sin(y);
  }

  // Compute Real Cepstrum Of x
  static void RealCepstrum(const std::span<double> x) {
    const auto n = x.size();

    auto real_time = std::vector<double>(n);
    auto imag_time = std::vector<double>(n);
    auto real_freq = std::vector<double>(n);
    auto imag_freq = std::vector<double>(n);

    // Compose Complex FFT Input
    std::ranges::copy(x, real_time.begin());
    std::ranges::fill(imag_time, 0.0);

    const auto real_time_span = std::span(real_time);
    const auto imag_time_span = std::span(imag_time);
    const auto real_freq_span = std::span(real_freq);
    const auto imag_freq_span = std::span(imag_freq);
    DFT(real_time_span, imag_time_span, real_freq_span, imag_freq_span);

    // Note: For real cepstrum, we only return the real part
    // The imaginary part should be negligible (numerical errors only)
    for (auto [real_sample, imag_sample] :
         std::views::zip(real_freq, imag_freq)) {
      // Calculate magnitude: sqrt(real^2 + imag^2)
      const double magnitude = sqrt(real_sample * real_sample + imag_sample * imag_sample);

      // Take natural log (add small epsilon to avoid log(0))
      const double kEpsilon = 1e-10;
      real_sample = log(magnitude + kEpsilon);
      imag_sample = 0.0;
    }

    // Perform Inverse FFT (this also scales by 1/n)
    InverseDFT(real_time_span, imag_time_span, real_freq_span, imag_freq_span);

    // Output Real Part Of FFT
    std::ranges::copy(real_time, x.begin());
  }

  // Compute Minimum Phase Reconstruction Of x
  static void MinimumPhase(const std::span<double> x) {
    const auto n = x.size();
    auto real_time = std::vector<double>(n);
    auto imag_time = std::vector<double>(n);
    auto real_freq = std::vector<double>(n);
    auto imag_freq = std::vector<double>(n);

    // Compose Complex FFT Input
    // keep DC component
    real_time[0] = x[0];
    for (const auto i : std::views::iota(1u, n)) {
      real_time[i] = x[i];
      imag_time[i] = 0.0;
    }

    // double the positive freqs (causal part)
    for (const auto i : std::views::iota(0u, n / 2)) {
      real_time[i] *= 2;
    }

    // nyquist freq (for even N)
    // todo assumes nyquist bin is half the input - is this correct?
    if (n % 2 == 0) {
      real_time[n / 2] = x[n / 2];
    }

    // zero out negative freqs (anti-causal part)
    for (const auto i : std::views::iota((n / 2) + 1, n)) {
      real_time[i] = 0;
    }

    const auto real_time_span = std::span(real_time);
    const auto imag_time_span = std::span(imag_time);
    const auto real_freq_span = std::span(real_freq);
    const auto imag_freq_span = std::span(imag_freq);
    DFT(real_time_span, imag_time_span, real_freq_span, imag_freq_span);

    // exponentiate to get complex spectrum
    for (auto [real_sample, imag_sample] :
         std::views::zip(real_freq, imag_freq)) {
      const double magnitude = exp(real_sample);
      const double phase = imag_sample;

      real_sample = magnitude * cos(phase);
      imag_sample = magnitude * sin(phase);
    }

    InverseDFT(real_time_span, imag_time_span, real_freq_span, imag_freq_span);

    std::ranges::copy(real_time, x.begin());
  }

  // FILTER ::::::
  void CreateLowPass(const double frequencyRatio) {
    const double proportionalRate =
        (frequencyRatio > 1.0) ? 0.5 / frequencyRatio : 0.5 * frequencyRatio;

    const double n = 1.0 / std::tan(juce::MathConstants<double>::twoPi *
                                    juce::jmax(0.001, proportionalRate));
    const double nSquared = n * n;
    const double c1 = 1.0 / (1.0 + std::sqrt(2.0) * n + nSquared);

    SetFilterCoefficients(c1, c1 * 2.0, c1, 1.0, c1 * 2.0 * (1.0 - nSquared),
                          c1 * (1.0 - std::sqrt(2.0) * n + nSquared));
  }
  void SetFilterCoefficients(double c1, double c2, double c3, double c4,
                             double c5, double c6) {
    const double a = 1.0 / c4;

    c1 *= a;
    c2 *= a;
    c3 *= a;
    c5 *= a;
    c6 *= a;

    coefficients_[0] = c1;
    coefficients_[1] = c2;
    coefficients_[2] = c3;
    coefficients_[3] = c4;
    coefficients_[4] = c5;
    coefficients_[5] = c6;
  }
  void ResetFilters() { filter_states_.clear(num_channels_); }
  void ApplyFilter(float* samples, int num, FilterState& fs) const {
    while (--num >= 0) {
      const double in = static_cast<double>(*samples);

      double out = coefficients_[0] * in + coefficients_[1] * fs.x1_ +
                   coefficients_[2] * fs.x2_ - coefficients_[4] * fs.y1_ -
                   coefficients_[5] * fs.y2_;

#if JUCE_INTEL
      if (!(out < -1.0e-8 || out > 1.0e-8)) out = 0;
#endif

      fs.x2_ = fs.x1_;
      fs.x1_ = in;
      fs.y2_ = fs.y1_;
      fs.y1_ = out;

      *samples++ = static_cast<float>(out);
    }
  }
  float FilterSample(float sample, FilterState& fs) const {
    const double in = static_cast<double>(sample);

    double out = coefficients_[0] * in + coefficients_[1] * fs.x1_ +
                 coefficients_[2] * fs.x2_ - coefficients_[4] * fs.y1_ -
                 coefficients_[5] * fs.y2_;

#if JUCE_INTEL
    if (!(out < -1.0e-8 || out > 1.0e-8)) out = 0;
#endif

    fs.x2_ = fs.x1_;
    fs.x1_ = in;
    fs.y2_ = fs.y1_;
    fs.y1_ = out;

    return static_cast<float>(out);
  }

  void Clear();
  [[nodiscard]] bool IsClear() const;

  // CUSTOM ::::
  void set_limiting_freq(float proportionOfSamplingRate);
  void set_aa_key_scaling(bool enable);

  void BuildBlep() const;
  void AddBlep(const BlepOffset& newBlep);

  void ProcessBlock(float* buffer, int numSamples);
  void ProcessCurrentBleps(float* buffer, int numSamples);
};

}  // namespace audio_plugin
