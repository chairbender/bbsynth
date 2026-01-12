#include "OTAFilterTPTNewtonRaphson.h"

import JuceImports;
import std;

#include "../Constants.h"
#include "../Utils.h"

namespace audio_plugin {
OTAFilterTPTNewtonRaphson::OTAFilterTPTNewtonRaphson(
    const juce::AudioBuffer<float>& env_buffer,
    const juce::AudioBuffer<float>& lfo_buffer)
    : cutoff_freq_{0.f},
      resonance_{0.f},
      drive_{0.f},
      env_mod_{0.f},
      lfo_mod_{0.f},
      num_stages_{4},
      env_buffer_{&env_buffer},
      lfo_buffer_{lfo_buffer},
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
  for (int i = 0; i < 4; ++i) {
    input_drive_scales_[static_cast<size_t>(i)] =
        state.getRawParameterValue("filterInputDriveScale" + juce::String(i + 1))
            ->load();
    state_drive_scales_[static_cast<size_t>(i)] =
        state.getRawParameterValue("filterStateDriveScale" + juce::String(i + 1))
            ->load();
  }
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

void OTAFilterTPTNewtonRaphson::Reset() {
  s1_ = s2_ = s3_ = s4_ = 0;
  for (auto& t : tanh_stages_) {
    t.reset();
  }
  for (auto& t : state_tanh_stages_) {
    t.reset();
  }
}

float OTAFilterTPTNewtonRaphson::EvaluateFilter(
    const float in, const float out_guess, const float G, const float k,
    float& v1_out, float& v2_out, float& v3_out, float& v4_out,
    const bool use_adaa) const {
  // Input with feedback
  const float u = in - k * out_guess;

  // Each stage: y = s + G * (sat(x) - sat(s))
  // where x is the input to the stage

  const float drive1 = drive_ * input_drive_scales_[0];
  const float state_drive1 = drive_ * state_drive_scales_[0];
  const float v1_sat = use_adaa ? tanh_stages_[0].process(u / drive1) * drive1
                                : std::tanh(u / drive1) * drive1;
  const float s1_sat = use_adaa ? state_tanh_stages_[0].process(s1_ / state_drive1) * state_drive1
                                : std::tanh(s1_ / state_drive1) * state_drive1;
  const float y1 = s1_ + G * (v1_sat - s1_sat);
  v1_out = y1;
  if (num_stages_ == 1) return y1;

  const float drive2 = drive_ * input_drive_scales_[1];
  const float state_drive2 = drive_ * state_drive_scales_[1];
  const float v2_sat = use_adaa ? tanh_stages_[1].process(y1 / drive2) * drive2
                                : std::tanh(y1 / drive2) * drive2;
  const float s2_sat = use_adaa ? state_tanh_stages_[1].process(s2_ / state_drive2) * state_drive2
                                : std::tanh(s2_ / state_drive2) * state_drive2;
  const float y2 = s2_ + G * (v2_sat - s2_sat);
  v2_out = y2;
  if (num_stages_ == 2) return y2;

  const float drive3 = drive_ * input_drive_scales_[2];
  const float state_drive3 = drive_ * state_drive_scales_[2];
  const float v3_sat = use_adaa ? tanh_stages_[2].process(y2 / drive3) * drive3
                                : std::tanh(y2 / drive3) * drive3;
  const float s3_sat = use_adaa ? state_tanh_stages_[2].process(s3_ / state_drive3) * state_drive3
                                : std::tanh(s3_ / state_drive3) * state_drive3;
  const float y3 = s3_ + G * (v3_sat - s3_sat);
  v3_out = y3;
  if (num_stages_ == 3) return y3;

  const float drive4 = drive_ * input_drive_scales_[3];
  const float state_drive4 = drive_ * state_drive_scales_[3];
  const float v4_sat = use_adaa ? tanh_stages_[3].process(y3 / drive4) * drive4
                                : std::tanh(y3 / drive4) * drive4;
  const float s4_sat = use_adaa ? state_tanh_stages_[3].process(s4_ / state_drive4) * state_drive4
                                : std::tanh(s4_ / state_drive4) * state_drive4;
  const float y4 = s4_ + G * (v4_sat - s4_sat);
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
  // Each stage: y = s + G * (sat(x) - sat(s))
  // dy/dx = G * d(sat(x))/dx
  // (Note: s is constant with respect to out_guess, so d(sat(s))/dx = 0)

  const float u = in - k * out_guess;

  const float drive1 = drive_ * input_drive_scales_[0];
  const float t1 = std::tanh(u / drive1);
  const float d_sat1 = 1.0f - t1 * t1;
  deriv = deriv * d_sat1;
  const float v1_sat = t1 * drive1;
  const float state_drive1 = drive_ * state_drive_scales_[0];
  const float s1_sat = std::tanh(s1_ / state_drive1) * state_drive1;
  const float y1 = s1_ + G * (v1_sat - s1_sat);
  deriv = G * deriv;
  if (num_stages_ == 1) return deriv;

  const float drive2 = drive_ * input_drive_scales_[1];
  const float t2 = std::tanh(y1 / drive2);
  const float d_sat2 = 1.0f - t2 * t2;
  deriv = deriv * d_sat2;
  const float v2_sat = t2 * drive2;
  const float state_drive2 = drive_ * state_drive_scales_[1];
  const float s2_sat = std::tanh(s2_ / state_drive2) * state_drive2;
  const float y2 = s2_ + G * (v2_sat - s2_sat);
  deriv = G * deriv;
  if (num_stages_ == 2) return deriv;

  const float drive3 = drive_ * input_drive_scales_[2];
  const float t3 = std::tanh(y2 / drive3);
  const float d_sat3 = 1.0f - t3 * t3;
  deriv = deriv * d_sat3;
  const float v3_sat = t3 * drive3;
  const float state_drive3 = drive_ * state_drive_scales_[2];
  const float s3_sat = std::tanh(s3_ / state_drive3) * state_drive3;
  const float y3 = s3_ + G * (v3_sat - s3_sat);
  deriv = G * deriv;
  if (num_stages_ == 3) return deriv;

  const float drive4 = drive_ * input_drive_scales_[3];
  const float t4 = std::tanh(y3 / drive4);
  const float d_sat4 = 1.0f - t4 * t4;
  deriv = deriv * d_sat4;
  deriv = G * deriv;

  return deriv;
}

float OTAFilterTPTNewtonRaphson::ProcessSample(const float in, const int index) {
  const auto env_data = env_buffer_->getReadPointer(0);
  const auto lfo_data = lfo_buffer_.getReadPointer(0);
  const float modulated_cutoff = juce::jlimit(
      kMinCutoff, kMaxCutoff,
      cutoff_freq_ + env_mod_ * env_data[index / kOversample] * kMaxCutoff +
          lfo_mod_ * lfo_data[index / kOversample] * kMaxCutoff);

  // Calculate TPT coefficient
  const float g = std::tanf(juce::MathConstants<float>::pi * modulated_cutoff /
                            sample_rate_);
  const float g_clamped = std::min(g, 0.9f);
  const float G = g_clamped / (1.0f + g_clamped);

  // Resonance feedback amount (scaled for 4-pole)
  const float k = std::clamp(resonance_, 0.0f, 0.99f) * 4.0f;

  // Newton-Raphson iteration to solve implicit equation
  // We're solving: out = F(input, out)
  // Or equivalently: out - F(input, out) = 0

  float out_guess = 0;
  switch (num_stages_) {
    case 1: out_guess = s1_; break;
    case 2: out_guess = s2_; break;
    case 3: out_guess = s3_; break;
    case 4: out_guess = s4_; break;
    default: out_guess = s4_; break;
  }
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
      Sanitize(EvaluateFilter(in, out_guess, G, k, v1, v2, v3, v4, true));

  // Update states using the converged solution
  // State update: s_new = 2*y - s_old
  // In TPT with sat: s_new = s_old + 2 * G * (sat(x) - sat(s_old))
  const float u = in - k * final_out;
  const float drive1 = drive_ * input_drive_scales_[0];
  const float state_drive1 = drive_ * state_drive_scales_[0];
  const float v1_sat = std::tanh(u / drive1) * drive1;
  const float s1_sat = std::tanh(s1_ / state_drive1) * state_drive1;
  s1_ = Sanitize(s1_ + 2.0f * G * (v1_sat - s1_sat));
  if (num_stages_ == 1) return final_out;

  const float drive2 = drive_ * input_drive_scales_[1];
  const float state_drive2 = drive_ * state_drive_scales_[1];
  const float v2_sat = std::tanh(v1 / drive2) * drive2;
  const float s2_sat = std::tanh(s2_ / state_drive2) * state_drive2;
  s2_ = Sanitize(s2_ + 2.0f * G * (v2_sat - s2_sat));
  if (num_stages_ == 2) return final_out;

  const float drive3 = drive_ * input_drive_scales_[2];
  const float state_drive3 = drive_ * state_drive_scales_[2];
  const float v3_sat = std::tanh(v2 / drive3) * drive3;
  const float s3_sat = std::tanh(s3_ / state_drive3) * state_drive3;
  s3_ = Sanitize(s3_ + 2.0f * G * (v3_sat - s3_sat));
  if (num_stages_ == 3) return final_out;

  const float drive4 = drive_ * input_drive_scales_[3];
  const float state_drive4 = drive_ * state_drive_scales_[3];
  const float v4_sat = std::tanh(v3 / drive4) * drive4;
  const float s4_sat = std::tanh(s4_ / state_drive4) * state_drive4;
  s4_ = Sanitize(s4_ + 2.0f * G * (v4_sat - s4_sat));

  return final_out;
}

void OTAFilterTPTNewtonRaphson::Process(juce::AudioBuffer<float>& buffers,
                                        const int start_sample,
                                        const int numSamples) {
  const auto data = buffers.getWritePointer(0);
  for (auto i = start_sample; i < start_sample + numSamples; ++i) {
    data[i] = ProcessSample(data[i], i);
  }
}

}  // namespace audio_plugin