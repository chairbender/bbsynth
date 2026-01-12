import JuceImports;
import std;

#include "AnalogADSR.h"

namespace audio_plugin {

void AnalogADSR::Prepare(const double sample_rate) {
  sample_rate_ = static_cast<float>(sample_rate);
}
void AnalogADSR::Configure(const float attack_seconds,
                           const float decay_seconds,
                           const float sustain_level,
                           const float release_seconds) {
  attack_samples_ = static_cast<int>(attack_seconds * sample_rate_);
  decay_samples_ = static_cast<int>(decay_seconds * sample_rate_);
  sustain_level_ = sustain_level;
  release_samples_ = static_cast<int>(release_seconds * sample_rate_);
}

void AnalogADSR::Reset() {
  state_ = State::idle;
  stage_samples_ = 0.0f;
  last_level_ = 0.f;
}

void AnalogADSR::NoteOn() {
  // reset to attack state regardless of where we are
  // todo: probably not thread safe
  state_ = State::attack;
  stage_samples_ = 0;
}

void AnalogADSR::NoteOff() {
  if (state_ != State::idle) {
    AdvanceStateFromSustain();
  }
}

void AnalogADSR::AdvanceStateFromAttack() {
  if (decay_samples_ > 0) {
    state_ = State::decay;
    stage_samples_ = 0;
  } else {
    AdvanceStateFromDecay();
  }
}
void AnalogADSR::AdvanceStateFromDecay() {
  if (sustain_level_ > 0.0f) {
    state_ = State::sustain;
    stage_samples_ = 0;
  } else {
    AdvanceStateFromSustain();
  }
}
void AnalogADSR::AdvanceStateFromSustain() {
  if (release_samples_ > 0) {
    state_ = State::release;
    stage_samples_ = 0;
    released_level_ = last_level_;
  } else {
    AdvanceStateFromRelease();
  }
}
void AnalogADSR::AdvanceStateFromRelease() {
  Reset();
}

// TODO: probably all the below code can use compile-time logic more to reduce runtime computation...

template<auto NextStateFunc,auto Curve,auto ConfiguredStageSamples,auto StartVal,auto TargetVal>
void AnalogADSR::WriteStage(juce::AudioBuffer<float>& buffer,
                                       const int start_sample,
                                       const int num_samples) {
  // todo: the taper(...) template function might be a better option here
  // todo: smarter / more efficient corve function.
  // todo: a lot of this is really inefficient
  const auto remaining_stage_samples = (this->*ConfiguredStageSamples) - stage_samples_;
  if (remaining_stage_samples <= 0.0) {
    // we started this buffer immediately transitioning to the next state,
    // so do the transition and continue writing using the new state
    (this->*NextStateFunc)();
    WriteEnvelopeToBuffer(buffer, start_sample, num_samples);
  } else {
    // todo: do a lot more of this calculation up front, not for every sample
    const auto excess_samples = num_samples - remaining_stage_samples;
    const auto samples_to_write =
        excess_samples >= 0 ? remaining_stage_samples : num_samples;
    // todo: could be constexpr if we find a library with a constexpr pow function
    const auto denom = std::pow(2, Curve) - 1;
    for (int i = 0; i < samples_to_write; ++i) {
      // scale time to 0,1 interval
      const float progress = (static_cast<float>(stage_samples_++) /
          static_cast<float>((this->*ConfiguredStageSamples)));
      const auto num = (std::pow(2, Curve * progress) - 1);
      const auto unscaled = num / denom;
      // now scale to target -> release (either of which may be a pointer to member or a constant)
      const float start_actual = [](const auto* self) {
        if constexpr (std::is_member_object_pointer_v<decltype(StartVal)>) {
          return (self->*StartVal);
        } else {
          return StartVal;
        }
      }(this);
      const float target_actual = [](const auto* self) {
        if constexpr (std::is_member_object_pointer_v<decltype(TargetVal)>) {
          return (self->*TargetVal);
        } else {
          return TargetVal;
        }
      }(this);
      // now scale the 0 - 1 interval to start - end
      const auto scaled = start_actual + (target_actual - start_actual) * static_cast<float>(unscaled);
      buffer.setSample(0, start_sample + i, scaled);
    }
    if (excess_samples > 0) {
      // we didn't fill the buffer up - transition to the next state and
      // continue rendering
      (this->*NextStateFunc)();
      WriteEnvelopeToBuffer(buffer, start_sample + samples_to_write, num_samples - samples_to_write);
    }
  }
  last_level_ = buffer.getSample(0, start_sample + num_samples - 1);
}

void AnalogADSR::WriteEnvelopeToBuffer(juce::AudioBuffer<float>& buffer,
                                       const int start_sample,
                                       const int num_samples) {
  if (state_ == State::idle) {
    buffer.clear(0, start_sample, num_samples);
    return;
  }
  if (state_ == State::attack) {
    WriteStage<&AnalogADSR::AdvanceStateFromAttack,-0.4f, &AnalogADSR::attack_samples_, 0.f, 1.f>
      (buffer, start_sample, num_samples);
  } else if (state_ == State::decay) {
    WriteStage<&AnalogADSR::AdvanceStateFromDecay,0.4f, &AnalogADSR::decay_samples_, 1.f, &AnalogADSR::sustain_level_>
      (buffer, start_sample, num_samples);
  } else if (state_ == State::sustain) {
    // sustain lasts until note off, so just write constant value
    const auto buffer_ptr = buffer.getWritePointer(0);
    for (int i = 0; i < num_samples; ++i) {
      buffer_ptr[i] = sustain_level_;
    }
  } else if (state_ == State::release) {
    WriteStage<&AnalogADSR::AdvanceStateFromRelease,0.4f, &AnalogADSR::release_samples_, &AnalogADSR::released_level_, 0.f>(buffer, start_sample, num_samples);
  }
}
bool AnalogADSR::IsActive() const {
  return state_ != State::idle;
}

}  // namespace audio_plugin