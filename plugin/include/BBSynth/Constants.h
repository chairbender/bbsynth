#pragma once

namespace audio_plugin {
constexpr auto kOversample = 2;
// at drive slider of "0" we still want SOME drive - the "natural" drive of the OTA.
// Having 0 actual drive creates instability;
constexpr auto kMinDrive = 1.f;
constexpr auto kMinCutoff = 20.0f;
// todo: I think it doesn't serve much purpose to allow it to go higher than the nyquist freq?
constexpr auto kMaxCutoff = 22000.0f;
}