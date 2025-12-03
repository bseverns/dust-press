#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "NativeDustPress.h"

class DustPressAudioProcessor : public juce::AudioProcessor {
public:
  DustPressAudioProcessor();
  ~DustPressAudioProcessor() override = default;

  // AudioProcessor overrides
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
  void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

  juce::AudioProcessorEditor* createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "DustPress"; }

  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

private:
  using APVTS = juce::AudioProcessorValueTreeState;
  using ParameterLayout = APVTS::ParameterLayout;

  static ParameterLayout createParameterLayout();

  void refreshLatencyFromEngine();
  void syncParametersToEngine();
  void resizeScratchBuffers(int numSamples);
  void primeEngineForBlock(int numSamples);

  struct ParameterRefs {
    std::atomic<float>* driveDb = nullptr;
    std::atomic<float>* bias = nullptr;
    std::atomic<float>* curveIndex = nullptr;
    std::atomic<float>* chaos = nullptr;
    std::atomic<float>* envToDriveDb = nullptr;
    std::atomic<float>* gateComp = nullptr;
    std::atomic<float>* preTiltDbPerOct = nullptr;
    std::atomic<float>* postAirDb = nullptr;
    std::atomic<float>* dirtAmount = nullptr;
    std::atomic<float>* ceilingDb = nullptr;
    std::atomic<float>* outputTrimDb = nullptr;
    std::atomic<float>* mixPercent = nullptr;
  } params;

  APVTS parameters;
  NativeDustPress engine;

  double lastSampleRate = 48000.0;
  int lastBlockSize = 0;

  std::vector<float> scratchLeft;
  std::vector<float> scratchRight;
};

