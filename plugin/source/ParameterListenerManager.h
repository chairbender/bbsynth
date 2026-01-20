#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace audio_plugin {

/**
 * Mixin to simplify registering parameter listeners using lambdas with dirty
 * tracking.
 * @tparam EnumType Enum representing parameter IDs. Must have a Count member.
 */
template <typename Derived, typename EnumType>
class ParameterListenerManager {
  static_assert(static_cast<int>(EnumType::kCount) <= 64,
                "ParameterListenerManager only supports up to 64 parameters.");

 public:
  explicit ParameterListenerManager(juce::AudioProcessorValueTreeState& apvts)
      : apvts_{apvts} {
    for (auto& val : latest_values_) {
      val.store(0.0f, std::memory_order_relaxed);
    }
  }

  void AddParameterListener(const juce::String& param_id, EnumType enum_id,
                            std::function<void(float)> callback) {
    const auto index = static_cast<size_t>(enum_id);
    if (callbacks_[index]) {
      DBG("duplicate parameter registration for enum index " << index);
    }
    callbacks_[index] = std::move(callback);

    auto listener = std::make_unique<LambdaParameterListener>(
        [this, index](float value) {
          latest_values_[index].store(value, std::memory_order_relaxed);
          dirty_flags_.fetch_or(1ULL << index, std::memory_order_release);
        });

    apvts_.addParameterListener(param_id, listener.get());
    parameter_listeners_[param_id] = std::move(listener);
  }

  /**
   * Processes all pending updates. Should be called at the start of the audio
   * block.
   */
  void ProcessDirtyParameters() {
    uint64_t dirty = dirty_flags_.exchange(0, std::memory_order_acquire);

    while (dirty) {
      // todo: needed?
#if defined(_MSC_VER)
      unsigned long index;
      if (_BitScanForward64(&index, dirty)) {
        dirty &= dirty - 1;
#else
      const int index = __builtin_ctzll(dirty);
      dirty &= dirty - 1;
#endif
      if (callbacks_[index]) {
        callbacks_[index](latest_values_[index].load(std::memory_order_relaxed));
      }
#if defined(_MSC_VER)
    }
#endif
    }
  }

  /**
   * Invokes all listeners, passing them the current value of the parameter.
   */
  void InitializeAllParameters() {
    for (const auto& [param_id, listener] : parameter_listeners_) {
      // Trigger the listener which will set the dirty flag and latest value
      listener->ParameterChanged(
          param_id, apvts_.getRawParameterValue(param_id)->load());
    }
    // Immediately process them so the state is initialized
    ProcessDirtyParameters();
  }

  void ClearAllListeners(juce::AudioProcessorValueTreeState& apvts) {
    for (auto& [paramID, listener] : parameter_listeners_) {
      apvts.removeParameterListener(paramID, listener.get());
    }
    parameter_listeners_.clear();
    for (auto& cb : callbacks_) {
      cb = nullptr;
    }
  }

 protected:
  ~ParameterListenerManager() = default;

 private:
  struct LambdaParameterListener
      : juce::AudioProcessorValueTreeState::Listener {
    explicit LambdaParameterListener(std::function<void(float)> callback)
        : on_parameter_changed_(std::move(callback)) {}

    void ParameterChanged(const juce::String& /*parameterID*/,
                          const float newValue) override {
      on_parameter_changed_(newValue);
    }

   private:
    std::function<void(float)> on_parameter_changed_;
  };

  std::map<juce::String, std::unique_ptr<LambdaParameterListener>>
      parameter_listeners_;
  std::array<std::function<void(float)>, static_cast<size_t>(EnumType::kCount)>
      callbacks_;
  std::array<std::atomic<float>, static_cast<size_t>(EnumType::kCount)>
      latest_values_;
  std::atomic<uint64_t> dirty_flags_{0};
  juce::AudioProcessorValueTreeState& apvts_;
};

}  // namespace audio_plugin
