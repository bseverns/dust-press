#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "NativeDustPress.h"

class DustPressAudioProcessor : public juce::AudioProcessor {
public:
  DustPressAudioProcessor();
  ~DustPressAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "DustPress"; }

  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
  const juce::String getProgramName(int index) override {
    juce::ignoreUnused(index);
    return "Default";
  }
  void changeProgramName(int index, const juce::String& newName) override {
    juce::ignoreUnused(index, newName);
  }

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState parameters;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  NativeDustPress processor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DustPressAudioProcessor)
};
