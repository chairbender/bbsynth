#include "BBSynth/OTAFilterTPTNewtonRaphson.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>

#include "BBSynth/Constants.h"

namespace audio_plugin {
OTAFilterTPTNewtonRaphson::OTAFilterTPTNewtonRaphson()
    : cutoff_freq_{0.f},
      resonance_{0.f},
      drive_{0.f},
      env_mod_{0.f},
      lfo_mod_{0.f},
      num_stages_{4},
      bypass_{false},
      sample_rate_{0},
      s1_{0},
      s2_{0},
      s3_{0},
      s4_{0} {}

void OTAFilterTPTNewtonRaphson::Configure(
    const juce::AudioProcessorValueTreeState& state) {
  cutoff_freq_ = state.getRawParameterValue("filterCutoffFreq")->load();
  resonance_ = state.getRawParameterValue("filterResonance")->load();
  drive_ = state.getRawParameterValue("filterDrive")->load();
  env_mod_ = state.getRawParameterValue("filterEnvMod")->load();
  lfo_mod_ = state.getRawParameterValue("filterLfoMod")->load();
  bypass_ = state.getRawParameterValue("vcfBypass")->load() > 0.5f;
  switch (static_cast<int>(state.getRawParameterValue("filterSlope")->load())) {
    case 0:
      num_stages_ = 4;
      break;
    case 1:
      num_stages_ = 3;
      break;
    case 2:
      num_stages_ = 2;
      break;
    default:
      num_stages_ = 4;
      break;
  }
}

void OTAFilterTPTNewtonRaphson::set_sample_rate(const double rate) {
  sample_rate_ = static_cast<float>(rate);
}

void OTAFilterTPTNewtonRaphson::Reset() { s1_ = s2_ = s3_ = s4_ = 0; }

float OTAFilterTPTNewtonRaphson::Saturate(const float x) const {
  return std::tanh(x / drive_) * drive_;
}

float OTAFilterTPTNewtonRaphson::SaturateDerivative(const float x) const {
  const float t = std::tanh(x / drive_);
  return 1.0f - t * t;  // sech^2 = 1 - tanh^2
}

float OTAFilterTPTNewtonRaphson::EvaluateFilter(
    const float in, const float out_guess, const float G, const float k,
    float& v1_out, float& v2_out, float& v3_out, float& v4_out) const {
  // Input with feedback
  const float u = in - k * out_guess;

  // TODO: refactor dupe
  const float v1_sat = Saturate(u);
  const float y1 = s1_ + G * (v1_sat - s1_);
  v1_out = y1;

  const float v2_sat = Saturate(y1);
  const float y2 = s2_ + G * (v2_sat - s2_);
  v2_out = y2;

  const float v3_sat = Saturate(y2);
  const float y3 = s3_ + G * (v3_sat - s3_);
  v3_out = y3;

  const float v4_sat = Saturate(y3);
  const float y4 = s4_ + G * (v4_sat - s4_);
  v4_out = y4;

  return y4;
}

float OTAFilterTPTNewtonRaphson::ComputeJacobian(const float in,
                                                 const float out_guess,
                                                 const float G,
                                                 const float k) const {
  // the derivative of the entire filter chain with respect to out_guess

  // Start with feedback: d(u)/d(out_guess) = -k
  float deriv = -k;

  // Chain through each stage
  // Each stage: y = s + G * (sat(x) - s)
  // dy/dx = G * d(sat(x))/dx

  const float u = in - k * out_guess;

  // TODO: refactor dupe
  const float d_sat1 = SaturateDerivative(u);
  deriv = deriv * d_sat1;
  const float v1_sat = Saturate(u);
  const float y1 = s1_ + G * (v1_sat - s1_);
  deriv = G * deriv;

  const float d_sat2 = SaturateDerivative(y1);
  deriv = deriv * d_sat2;
  const float v2_sat = Saturate(y1);
  const float y2 = s2_ + G * (v2_sat - s2_);
  deriv = G * deriv;

  const float d_sat3 = SaturateDerivative(y2);
  deriv = deriv * d_sat3;
  const float v3_sat = Saturate(y2);
  const float y3 = s3_ + G * (v3_sat - s3_);
  deriv = G * deriv;

  const float d_sat4 = SaturateDerivative(y3);
  deriv = deriv * d_sat4;
  deriv = G * deriv;

  return deriv;
}

float OTAFilterTPTNewtonRaphson::ProcessSample(const float in) {
  // Calculate TPT coefficient
  const float g =
      std::tanf(juce::MathConstants<float>::pi * cutoff_freq_ / sample_rate_);
  const float g_clamped = std::min(g, 0.9f);
  const float G = g_clamped / (1.0f + g_clamped);

  // Resonance feedback amount (scaled for 4-pole)
  const float k = std::clamp(resonance_, 0.0f, 0.99f) * 4.0f;

  // Newton-Raphson iteration to solve implicit equation
  // We're solving: out = F(input, out)
  // Or equivalently: out - F(input, out) = 0

  float out_guess = s4_;             // Initial guess: previous output
  constexpr int max_iterations = 4;  // Usually converges in 2-3 iterations
  constexpr float tolerance = 1e-6f;

  float v1, v2, v3, v4;  // Stage outputs

  for (int iter = 0; iter < max_iterations; ++iter) {
    // Evaluate what output would be for this guess
    const float predicted_out =
        EvaluateFilter(in, out_guess, G, k, v1, v2, v3, v4);

    // Residual: how far off is our prediction?
    // We want: out_guess = predicted_out
    // So residual = out_guess - predicted_out
    const float residual = out_guess - predicted_out;

    // Check convergence
    if (std::abs(residual) < tolerance) {
      break;
    }

    // Compute Jacobian (derivative)
    // J = d(out_guess - F(input, out_guess))/d(out_guess)
    //   = 1 - dF/d(out_guess)
    const float dF = ComputeJacobian(in, out_guess, G, k);
    const float jacobian = 1.0f - dF;

    // Avoid division by zero
    if (std::abs(jacobian) < 1e-10f) {
      break;
    }

    // Newton-Raphson update
    out_guess -= residual / jacobian;

    // Clamp to reasonable range to prevent divergence
    out_guess = std::clamp(out_guess, -10.0f, 10.0f);
  }

  // Final evaluation with converged output
  const float final_out =
      EvaluateFilter(in, out_guess, G, k, v1, v2, v3, v4);

  // Update states using the converged solution
  // State update: s_new = 2*y - s_old
  // TODO: reduce dupe
  const float u = in - k * final_out;
  const float v1_sat = Saturate(u);
  s1_ += 2.0f * G * (v1_sat - s1_);

  const float v2_sat = Saturate(v1);
  s2_ += 2.0f * G * (v2_sat - s2_);

  const float v3_sat = Saturate(v2);
  s3_ += 2.0f * G * (v3_sat - s3_);

  const float v4_sat = Saturate(v3);
  s4_ += 2.0f * G * (v4_sat - s4_);

  return final_out;
}

void OTAFilterTPTNewtonRaphson::Process(
    juce::AudioBuffer<float>& buffers,
    [[maybe_unused]] const juce::AudioBuffer<float>& env_buffer,
    [[maybe_unused]] const juce::AudioBuffer<float>& lfo_buffer, int start_sample,
    int numSamples) {
  const auto data = buffers.getWritePointer(0);
  for (auto i = start_sample; i < start_sample + numSamples; ++i) {
    data[i] = ProcessSample(data[i]);
  }
}

}  // namespace audio_plugin