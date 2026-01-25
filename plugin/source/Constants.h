#pragma once
#include <format>
#include <ranges>

namespace audio_plugin {
constexpr auto kOversample = 1;
constexpr double kBlepOversampleRatio = 16;
constexpr double kBlepZeroCrossings = 16;
// proportion of sample rate.
// TODO: Try adjusting to .125 and see what happens
constexpr auto kBlepProportionalFreq = 0.5;
// todo: may change depending on values in MinBlepGenerator
constexpr auto kBlepTableSize = 512;
// at drive slider of "0" we still want SOME drive - the "natural" drive of the OTA.
// Having 0 actual drive creates instability;
constexpr auto kMinDrive = .5f;
constexpr auto kMinCutoff = 20.0f;
// todo: I think it doesn't serve much purpose to allow it to go higher than the nyquist freq?
constexpr auto kMaxCutoff = 22000.0f;
constexpr auto kNumVoices = 4;

// todo: not sure if putting a function here is a best practice...
constexpr auto GetIndexSuffixedParams(const std::string_view param_prefix) {
  return std::views::iota(0, 4) | std::views::transform([param_prefix](auto i) {
           return std::format("{}{}", param_prefix, i + 1);
         });
}
constexpr auto kInputDriveScaleParams =
    GetIndexSuffixedParams("filterInputDriveScale");
constexpr auto kStateDriveScaleParams =
    GetIndexSuffixedParams("filterStateDriveScale");
}