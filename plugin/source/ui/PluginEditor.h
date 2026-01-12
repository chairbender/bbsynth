#pragma once

import JuceImports;

#include "../PluginProcessor.h"
#include "SpectrumAnalyzerComponent.h"
#include "section/Env1Section.h"
#include "section/Env2Section.h"
#include "section/LFOSection.h"
#include "section/VCASection.h"
#include "section/VCFDriveScalingSection.h"
#include "section/VCFSection.h"
#include "section/VCO1Section.h"
#include "section/VCO2Section.h"
#include "section/VCOModSection.h"

namespace audio_plugin {

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor {
 public:
  explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
  /**
   * must be called by root component to update audio-reactive components
   */
  void GetNextAudioBlock(juce::AudioBuffer<float>& buffer);
  ~AudioPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  juce::Grid LayoutMainGrid();
  void resized() override;
  // todo refactor so this can be private
  juce::MidiKeyboardState keyboard_state_;

 private:
  static juce::Grid MakeMainGrid();
  void PaintBackground(juce::Graphics& g) const;

  juce::MidiKeyboardComponent keyboard_component_;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processor_ref_;

  LFOSection lfo_section_;
  VCOModSection vco_mod_section_;
  VCO1Section vco1_section_;
  VCO2Section vco2_section_;
  VCFSection vcf_section_;
  VCFDriveScalingSection vcf_drive_scaling_section_;
  VCASection vca_section_;
  Env1Section env1_section_;
  Env2Section env2_section_;

  SpectrumAnalyzerComponent spectrum_analyzer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
}  // namespace audio_plugin
