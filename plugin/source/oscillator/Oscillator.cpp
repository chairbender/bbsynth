#include "Oscillator.h"

#include <ranges>
#include <span>

#include "../Constants.h"
#include "../Utils.h"

namespace audio_plugin {

OscillatorSound::OscillatorSound(
    [[maybe_unused]] juce::AudioProcessorValueTreeState& apvts) {}

bool OscillatorSound::appliesToNote([[maybe_unused]] int midiNoteIndex) {
  return true;
}

bool OscillatorSound::appliesToChannel([[maybe_unused]] int midiChannelIndex) {
  return true;
}

OscillatorVoice::OscillatorVoice(juce::AudioProcessorValueTreeState& apvts,
                                 const juce::AudioBuffer<float>& lfo_buffer)
    : ParameterListenerManager{apvts},
      lfo_buffer_{lfo_buffer},
      waveGenerator_{lfo_buffer_, env1_buffer_, env2_buffer_, wave2_buffer_,
                     hard_sync_reset_sample_indices_},
      wave2Generator_{lfo_buffer_, env1_buffer_, env2_buffer_, wave2_buffer_,
                      hard_sync_reset_sample_indices_},
      filter_tpt_{apvts, env1_buffer_, lfo_buffer_},
      filter_dfb_{apvts, env1_buffer_, lfo_buffer_} {
  waveGenerator_.PrepareToPlay(getSampleRate() * kOversample);
  wave2Generator_.PrepareToPlay(getSampleRate() * kOversample);
  waveGenerator_.set_mode(ANTIALIAS);
  wave2Generator_.set_mode(ANTIALIAS);
  filter_tpt_.set_sample_rate(getSampleRate() * kOversample);
  filter_dfb_.set_sample_rate(getSampleRate() * kOversample);

  AddParameterListener("vcfFilterType", OscillatorVoiceParamId::kVcfFilterType,
                       [this](const float value) {
                         filter_type_ = static_cast<int>(value);
                       });

  // ADSR 1
  AddParameterListener("adsrAttack", OscillatorVoiceParamId::kAdsrAttack,
                       [this](const float value) {
                         envelope_.set_attack(value);
                       });
  AddParameterListener("adsrDecay", OscillatorVoiceParamId::kAdsrDecay,
                       [this](const float value) {
                         envelope_.set_decay(value);
                       });
  AddParameterListener("adsrSustain", OscillatorVoiceParamId::kAdsrSustain,
                       [this](const float value) {
                         envelope_.set_sustain(value);
                       });
  AddParameterListener("adsrRelease", OscillatorVoiceParamId::kAdsrRelease,
                       [this](const float value) {
                         envelope_.set_release(value);
                       });
  AddParameterListener("env1RetriggerRate", OscillatorVoiceParamId::kEnv1RetriggerRate,
                       [this](const float value) {
                         envelope_.set_retrigger_constant_rate(value > 0.5f);
                       });

  // ADSR 2
  AddParameterListener("env2Attack", OscillatorVoiceParamId::kEnv2Attack,
                       [this](const float value) {
                         envelope2_.set_attack(value);
                       });
  AddParameterListener("env2Decay", OscillatorVoiceParamId::kEnv2Decay,
                       [this](const float value) {
                         envelope2_.set_decay(value);
                       });
  AddParameterListener("env2Sustain", OscillatorVoiceParamId::kEnv2Sustain,
                       [this](const float value) {
                         envelope2_.set_sustain(value);
                       });
  AddParameterListener("env2Release", OscillatorVoiceParamId::kEnv2Release,
                       [this](const float value) {
                         envelope2_.set_release(value);
                       });
  AddParameterListener("env2RetriggerRate", OscillatorVoiceParamId::kEnv2RetriggerRate,
                       [this](const float value) {
                         envelope2_.set_retrigger_constant_rate(value > 0.5f);
                       });

  // VCO Mod
  auto update_vco_mod = [this, &apvts]() {
    const float lfo_freq = apvts.getRawParameterValue("vcoModLfoFreq")->load();
    const float env1_freq = apvts.getRawParameterValue("vcoModEnv1Freq")->load();

    if (apvts.getRawParameterValue("vcoModOsc1")->load() > 0) {
      waveGenerator_.set_pitch_bend_lfo_mod(lfo_freq);
      waveGenerator_.set_pitch_bend_env1_mod(env1_freq);
    } else {
      waveGenerator_.set_pitch_bend_lfo_mod(0);
      waveGenerator_.set_pitch_bend_env1_mod(0);
    }

    if (apvts.getRawParameterValue("vcoModOsc2")->load() > 0) {
      wave2Generator_.set_pitch_bend_lfo_mod(lfo_freq);
      wave2Generator_.set_pitch_bend_env1_mod(env1_freq);
    } else {
      wave2Generator_.set_pitch_bend_lfo_mod(0);
      wave2Generator_.set_pitch_bend_env1_mod(0);
    }
  };

  AddParameterListener("vcoModOsc1", OscillatorVoiceParamId::kVcoModOsc1, [update_vco_mod](auto) { update_vco_mod(); });
  AddParameterListener("vcoModOsc2", OscillatorVoiceParamId::kVcoModOsc2, [update_vco_mod](auto) { update_vco_mod(); });
  AddParameterListener("vcoModLfoFreq", OscillatorVoiceParamId::kVcoModLfoFreq, [update_vco_mod](auto) { update_vco_mod(); });
  AddParameterListener("vcoModEnv1Freq", OscillatorVoiceParamId::kVcoModEnv1Freq, [update_vco_mod](auto) { update_vco_mod(); });
  AddParameterListener("antiAlias", OscillatorVoiceParamId::kAntiAlias,
                       [this](const float value) {
                         const auto mode = value > 0.5f ? ANTIALIAS : NO_ANTIALIAS;
                         waveGenerator_.set_mode(mode);
                         wave2Generator_.set_mode(mode);
                       });

  // Wave Types
  AddParameterListener("waveType", OscillatorVoiceParamId::kWaveType,
                       [this](const float value) {
                         switch (static_cast<int>(value)) {
                           case 0: waveGenerator_.set_wave_type(sine); break;
                           case 1: waveGenerator_.set_wave_type(sawFall); break;
                           case 2: waveGenerator_.set_wave_type(triangle); break;
                           case 3: waveGenerator_.set_wave_type(square); break;
                           case 4: waveGenerator_.set_wave_type(random); break;
                           default: DBG("unhandled default case");
                         }
                       });
  AddParameterListener("wave2Type", OscillatorVoiceParamId::kWave2Type,
                       [this](const float value) {
                         switch (static_cast<int>(value)) {
                           case 0: wave2Generator_.set_wave_type(sine); break;
                           case 1: wave2Generator_.set_wave_type(sawFall); break;
                           case 2: wave2Generator_.set_wave_type(triangle); break;
                           case 3: wave2Generator_.set_wave_type(square); break;
                           case 4: wave2Generator_.set_wave_type(random); break;
                           default: DBG("unhandled default case");
                         }
                       });

  // Sync / Fine / Cross
  auto update_sync_cross = [this, &apvts]() {
    const bool hard_sync = apvts.getRawParameterValue("vco2Sync")->load() > 0.5f;
    const float crossMod = apvts.getRawParameterValue("crossMod")->load();

    if (hard_sync && crossMod <= 0.f) {
      waveGenerator_.set_hard_sync_mode(PRIMARY);
      wave2Generator_.set_hard_sync_mode(SECONDARY);
    } else {
      waveGenerator_.set_hard_sync_mode(DISABLED);
      wave2Generator_.set_hard_sync_mode(DISABLED);
    }

    waveGenerator_.set_cross_mod(crossMod);
    if (crossMod > 0.f) {
      waveGenerator_.set_mode(NO_ANTIALIAS);
      wave2Generator_.set_mode(NO_ANTIALIAS);
    } else {
      waveGenerator_.set_mode(ANTIALIAS);
      wave2Generator_.set_mode(ANTIALIAS);
    }
  };

  AddParameterListener("vco2Sync", OscillatorVoiceParamId::kVco2Sync, [update_sync_cross](auto) { update_sync_cross(); });
  AddParameterListener("crossMod", OscillatorVoiceParamId::kCrossMod, [update_sync_cross](auto) { update_sync_cross(); });
  AddParameterListener("fineTune", OscillatorVoiceParamId::kFineTune,
                       [this](const float value) {
                         wave2Generator_.set_pitch_offset_semis(static_cast<double>(value));
                       });

  // Pulse Width
  auto update_pw = [this, &apvts]() {
    const int pulseWidthSource = static_cast<int>(apvts.getRawParameterValue("pulseWidthSource")->load());
    const auto pulseWidth = static_cast<double>(apvts.getRawParameterValue("pulseWidth")->load());

    const auto set_pw_type = [this](const PulseWidthModType type) {
      waveGenerator_.set_pulse_width_mod_type(type);
      wave2Generator_.set_pulse_width_mod_type(type);
    };

    switch (pulseWidthSource) {
      case 0: set_pw_type(env2Minus); break;
      case 1: set_pw_type(env2Plus); break;
      case 2: set_pw_type(env1Minus); break;
      case 3: set_pw_type(env1Plus); break;
      case 4: set_pw_type(lfo); break;
      case 5: set_pw_type(manual); break;
      default:;
    }
    waveGenerator_.set_pulse_width_mod(pulseWidth);
    wave2Generator_.set_pulse_width_mod(pulseWidth);
  };
  AddParameterListener("pulseWidthSource", OscillatorVoiceParamId::kPulseWidthSource, [update_pw](auto) { update_pw(); });
  AddParameterListener("pulseWidth", OscillatorVoiceParamId::kPulseWidth, [update_pw](auto) { update_pw(); });

  // Gain
  AddParameterListener("vco1Level", OscillatorVoiceParamId::kVco1Level,
                       [this](const float value) {
                         waveGenerator_.set_gain(static_cast<double>(value));
                       });
  AddParameterListener("vco2Level", OscillatorVoiceParamId::kVco2Level,
                       [this](const float value) {
                         wave2Generator_.set_gain(static_cast<double>(value));
                       });

  // Filter Env Source
  AddParameterListener("filterEnvSource", OscillatorVoiceParamId::kFilterEnvSource,
                       [this](const float value) {
                         if (static_cast<int>(value) == 0) {
                           filter_dfb_.set_env_buffer(env1_buffer_);
                           filter_tpt_.set_env_buffer(env1_buffer_);
                         } else {
                           filter_dfb_.set_env_buffer(env2_buffer_);
                           filter_tpt_.set_env_buffer(env2_buffer_);
                         }
                       });
}

void OscillatorVoice::PrepareToPlay() {
  InitializeAllParameters();
  envelope_.Prepare(getSampleRate());
  envelope2_.Prepare(getSampleRate());
  filter_tpt_.InitializeAllParameters();
  filter_dfb_.InitializeAllParameters();
}

bool OscillatorVoice::canPlaySound(juce::SynthesiserSound* sound) {
  return dynamic_cast<OscillatorSound*>(sound) != nullptr;
}

void OscillatorVoice::SetBlockSize(const int blockSize) {
  downsampler_.prepare(blockSize, kOversample);
  const auto oversample_samples = blockSize * kOversample;
  oversample_buffer_.setSize(1, oversample_samples, false, true);
  wave2_buffer_.setSize(1, oversample_samples, false, true);
  env1_buffer_.setSize(1, blockSize, false, true);
  env2_buffer_.setSize(1, blockSize, false, true);
}

void OscillatorVoice::startNote(const int midiNoteNumber,
                                [[maybe_unused]] const float velocity,
                                [[maybe_unused]] juce::SynthesiserSound* sound,
                                [[maybe_unused]] int pitchWheelPos) {
  const auto smooth = envelope_.IsActive();
  waveGenerator_.set_pitch_semitone(midiNoteNumber, getSampleRate(), smooth);
  wave2Generator_.set_pitch_semitone(midiNoteNumber, getSampleRate(), smooth);
  envelope_.NoteOn();
  envelope2_.NoteOn();
}

void OscillatorVoice::stopNote([[maybe_unused]] float velocity,
                               [[maybe_unused]] const bool allowTailOff) {
  envelope_.NoteOff();
  envelope2_.NoteOff();
}

void OscillatorVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {
}

void OscillatorVoice::controllerMoved([[maybe_unused]] int controllerNumber,
                                      [[maybe_unused]] int newControllerValue) {
}

void OscillatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      [[maybe_unused]] int startSample,
                                      const int numSamples) {
  ProcessDirtyParameters();
  const auto oversample_samples = numSamples * kOversample;
  ;
  const auto oversample_start_sample = startSample * kOversample;

  // TODO: how does this interact with note on? Does this mean envelope always
  //  starts at start of a block even if it "should" start mid-block?
  // fill envelope buffers
  envelope_.WriteEnvelopeToBuffer(env1_buffer_, startSample, numSamples);
  envelope2_.WriteEnvelopeToBuffer(env2_buffer_, startSample, numSamples);

  // note this will fill and process only the left channel since we want to work
  // in mono until the last moment the wave generator and filter are already
  // configured to generate at 2x oversampling..
  // we need wave2 first so we can use it for cross-mod (FM)
  // TODO: should the envelope actually affect the cross-mod behavior?
  // todo: add ability to tell RenderNextBlock to OVERWRITE instead of add
  //    I think we need this clear since wave generator will ADD so we can avoid
  //    the clearing

  // todo: when cross mod, we should disable hard sync for now. When hard sync,
  //  we need to evaluate generator 1 first as gen2 depends on it (knowing the
  //  reset sample indices).

  if (waveGenerator_.cross_mod() > 0) {
    // cross mod - need to run vco2 first so it can modulate vco1
    wave2_buffer_.clear(oversample_start_sample, oversample_samples);
    wave2Generator_.RenderNextBlock(wave2_buffer_, oversample_start_sample,
                                    oversample_samples);
    oversample_buffer_.clear(oversample_start_sample, oversample_samples);
    // todo: Do we even need this intermediate wave2_buffer? What if we
    //  cross-mod from the oversample_buffer_ directly? if we're doing FM, we
    //  only use wave 2 for FM, we don't output it directly
    waveGenerator_.RenderNextBlock(oversample_buffer_, oversample_start_sample,
                                   oversample_samples);
  } else {
    // no cross mod or hard sync, need to run generator 1 first as it
    // sets the reset points for generator 2
    oversample_buffer_.clear(oversample_start_sample, oversample_samples);
    waveGenerator_.RenderNextBlock(oversample_buffer_, oversample_start_sample,
                                   oversample_samples);
    wave2Generator_.RenderNextBlock(oversample_buffer_, oversample_start_sample,
                                    oversample_samples);
  }

  if (filter_type_ == 0) {
    filter_dfb_.Process(oversample_buffer_, oversample_start_sample,
                        oversample_samples);
  } else if (filter_type_ == 1) {
    filter_tpt_.Process(oversample_buffer_, oversample_start_sample,
                        oversample_samples);
  }

  // Apply ADSR envelope to the mono oversampled buffer (VCA)
  const auto data_span = std::span{oversample_buffer_.getWritePointer(0) + oversample_start_sample,
                                   static_cast<size_t>(oversample_samples)};
  const auto env1_data_span =
      std::span{env1_buffer_.getReadPointer(0) + startSample, static_cast<size_t>(numSamples)};

  for (const auto [sample, env_sample] :
       std::views::zip(data_span | std::views::chunk(kOversample), env1_data_span)) {
    for (auto& s : sample) {
      s *= env_sample;
    }
  }

  if (!envelope_.IsActive()) {
    // todo: might need this or no?
    // waveGenerator_.set_volume(-120);
    // wave2Generator_.set_volume(-120);
    clearCurrentNote();
  }

  downsampler_.process(oversample_buffer_, outputBuffer,
                       oversample_start_sample, oversample_samples);
}
}  // namespace audio_plugin