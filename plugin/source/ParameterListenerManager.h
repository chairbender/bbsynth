#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace audio_plugin {

template <typename Derived>
class ParameterListenerManager {
 public:
  explicit ParameterListenerManager(juce::AudioProcessorValueTreeState& apvts)
      : apvts_{apvts} {}

  void AddParameterListener(
      const juce::String& param_id,
      std::function<void(const juce::String&, float)> callback) {
    if (parameter_listeners_.contains(param_id))
      DBG("duplicate parameter registration " + param_id);
    auto listener = std::make_unique<LambdaParameterListener>(callback);
    apvts_.addParameterListener(param_id, listener.get());
    parameter_listeners_[param_id] = std::move(listener);
  }

  void RemoveParameterListener(const juce::String& paramID) {
    auto it = parameter_listeners_.find(paramID);
    if (it != parameter_listeners_.end()) {
      apvts_.removeParameterListener(paramID, it->second.get());
      parameter_listeners_.erase(it);
    }
  }

  /**
   * Invokes all listeners, passing them the current value of the parameter.
   */
  void InitializeAllParameters() {
    for (const auto& [param_id, listener] : parameter_listeners_)
      listener->ParameterChanged(param_id, apvts_.getRawParameterValue(param_id)->load());
  }

  void ClearAllListeners(juce::AudioProcessorValueTreeState& apvts) {
    for (auto& [paramID, listener] : parameter_listeners_) {
      apvts.removeParameterListener(paramID, listener.get());
    }
    parameter_listeners_.clear();
  }

 protected:
  ~ParameterListenerManager() = default;

 private:
  struct LambdaParameterListener
      : juce::AudioProcessorValueTreeState::Listener {
    explicit LambdaParameterListener(
        std::function<void(const juce::String&, float)> callback)
        : on_parameter_changed_(std::move(callback)) {}

    void ParameterChanged(const juce::String& parameter_id,
                          const float new_value) override {
      on_parameter_changed_(parameter_id, new_value);
    }

   private:
    std::function<void(const juce::String&, float)> on_parameter_changed_;
  };

  std::map<juce::String, std::unique_ptr<LambdaParameterListener>>
      parameter_listeners_;
  juce::AudioProcessorValueTreeState& apvts_;
};

}  // namespace audio_plugin
