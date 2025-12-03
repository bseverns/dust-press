#include "DustPressAudioProcessor.h"

namespace {
constexpr const char* kParamDriveDb = "drive_db";
constexpr const char* kParamBias = "bias";
constexpr const char* kParamCurve = "curve_index";
constexpr const char* kParamChaos = "chaos_level";
constexpr const char* kParamEnvToDrive = "env_to_drive_db";
constexpr const char* kParamGateComp = "gate_comp";
constexpr const char* kParamPreTilt = "pre_tilt_db_per_oct";
constexpr const char* kParamPostAir = "post_air_gain_db";
constexpr const char* kParamDirt = "dirt_amount";
constexpr const char* kParamCeiling = "limiter_ceiling_db";
constexpr const char* kParamOutputTrim = "out_trim_db";
constexpr const char* kParamMixPercent = "mix_percent";
}

DustPressAudioProcessor::DustPressAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DustPressParameters", createParameterLayout()) {
  params.driveDb = parameters.getRawParameterValue(kParamDriveDb);
  params.bias = parameters.getRawParameterValue(kParamBias);
  params.curveIndex = parameters.getRawParameterValue(kParamCurve);
  params.chaos = parameters.getRawParameterValue(kParamChaos);
  params.envToDriveDb = parameters.getRawParameterValue(kParamEnvToDrive);
  params.gateComp = parameters.getRawParameterValue(kParamGateComp);
  params.preTiltDbPerOct = parameters.getRawParameterValue(kParamPreTilt);
  params.postAirDb = parameters.getRawParameterValue(kParamPostAir);
  params.dirtAmount = parameters.getRawParameterValue(kParamDirt);
  params.ceilingDb = parameters.getRawParameterValue(kParamCeiling);
  params.outputTrimDb = parameters.getRawParameterValue(kParamOutputTrim);
  params.mixPercent = parameters.getRawParameterValue(kParamMixPercent);
}

juce::AudioProcessorValueTreeState::ParameterLayout
DustPressAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;

  auto driveRange = juce::NormalisableRange<float>(0.0f, 36.0f);
  driveRange.setSkewForCentre(12.0f);
  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamDriveDb, "Drive (dB)", driveRange, 12.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamBias, "Bias", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

  layout.push_back(std::make_unique<juce::AudioParameterChoice>(
      kParamCurve, "Curve",
      juce::StringArray{"tanh", "cubic", "diode", "fold"}, 0));

  juce::StringArray chaosLabels;
  chaosLabels.add("0 - clean");
  chaosLabels.add("1");
  chaosLabels.add("2");
  chaosLabels.add("3");
  chaosLabels.add("4");
  chaosLabels.add("5");
  chaosLabels.add("6");
  chaosLabels.add("7");
  layout.push_back(std::make_unique<juce::AudioParameterChoice>(kParamChaos, "Chaos", chaosLabels, 0));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamEnvToDrive, "Env to Drive (dB)", juce::NormalisableRange<float>(-12.0f, 12.0f), 6.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamGateComp, "GateComp", juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamPreTilt, "Pre Tilt (dB/oct)", juce::NormalisableRange<float>(-6.0f, 6.0f), 0.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamPostAir, "Post Air (dB)", juce::NormalisableRange<float>(-6.0f, 6.0f), 0.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamDirt, "Dirt", juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamCeiling, "Limiter Ceiling (dB)", juce::NormalisableRange<float>(-6.0f, 0.0f), -1.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamOutputTrim, "Output Trim (dB)", juce::NormalisableRange<float>(-12.0f, 6.0f), 0.0f));

  layout.push_back(std::make_unique<juce::AudioParameterFloat>(
      kParamMixPercent, "Mix %", juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

  return {layout.begin(), layout.end()};
}

void DustPressAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  lastSampleRate = sampleRate;
  lastBlockSize = samplesPerBlock;

  engine.setSampleRate(static_cast<float>(sampleRate));
  engine.reset();
  syncParametersToEngine();
  refreshLatencyFromEngine();
}

void DustPressAudioProcessor::releaseResources() {}

bool DustPressAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto mainIn = layouts.getMainInputChannelSet();
  const auto mainOut = layouts.getMainOutputChannelSet();
  return mainIn == juce::AudioChannelSet::stereo() && mainOut == juce::AudioChannelSet::stereo();
}

void DustPressAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  midiMessages.clear();

  const int numSamples = buffer.getNumSamples();
  if (numSamples == 0) {
    return;
  }

  primeEngineForBlock(numSamples);

  const auto numChannels = buffer.getNumChannels();
  auto* leftIn = buffer.getReadPointer(0);
  auto* rightIn = numChannels > 1 ? buffer.getReadPointer(1) : leftIn;

  if (numChannels > 1) {
    auto* leftOut = buffer.getWritePointer(0);
    auto* rightOut = buffer.getWritePointer(1);
    engine.processBlock(leftIn, rightIn, leftOut, rightOut, static_cast<std::size_t>(numSamples));
  } else {
    auto* leftOut = buffer.getWritePointer(0);

    for (int i = 0; i < numSamples; ++i) {
      const auto sample = leftIn[i];
      scratchLeft[static_cast<std::size_t>(i)] = sample;
      scratchRight[static_cast<std::size_t>(i)] = sample;
    }

    engine.processBlock(scratchLeft.data(), scratchRight.data(), scratchLeft.data(), scratchRight.data(),
                        static_cast<std::size_t>(numSamples));

    for (int i = 0; i < numSamples; ++i) {
      leftOut[i] = scratchLeft[static_cast<std::size_t>(i)];
    }
  }

  for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
    buffer.clear(ch, 0, numSamples);
  }
}

void DustPressAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                           juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  midiMessages.clear();

  const int numSamples = buffer.getNumSamples();
  if (numSamples == 0) {
    return;
  }

  primeEngineForBlock(numSamples);

  resizeScratchBuffers(numSamples);

  auto* leftIn = buffer.getReadPointer(0);
  auto* rightIn = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : buffer.getReadPointer(0);

  for (int i = 0; i < numSamples; ++i) {
    scratchLeft[static_cast<std::size_t>(i)] = static_cast<float>(leftIn[i]);
    scratchRight[static_cast<std::size_t>(i)] = static_cast<float>(rightIn[i]);
  }

  engine.processBlock(scratchLeft.data(), scratchRight.data(), scratchLeft.data(), scratchRight.data(),
                      static_cast<std::size_t>(numSamples));

  auto* leftOut = buffer.getWritePointer(0);
  auto* rightOut = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  for (int i = 0; i < numSamples; ++i) {
    leftOut[i] = static_cast<double>(scratchLeft[static_cast<std::size_t>(i)]);
    if (rightOut != nullptr) {
      rightOut[i] = static_cast<double>(scratchRight[static_cast<std::size_t>(i)]);
    }
  }

  for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
    buffer.clear(ch, 0, numSamples);
  }
}

void DustPressAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = parameters.copyState();
  if (auto xml = state.createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void DustPressAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
    if (xmlState->hasTagName(parameters.state.getType())) {
      parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
      syncParametersToEngine();
      refreshLatencyFromEngine();
    }
  }
}

void DustPressAudioProcessor::refreshLatencyFromEngine() {
  setLatencySamples(static_cast<int>(engine.getLatencySamples()));
}

void DustPressAudioProcessor::syncParametersToEngine() {
  engine.setDriveDb(params.driveDb ? *(params.driveDb) : 0.0f);
  engine.setBias(params.bias ? *(params.bias) : 0.0f);
  engine.setCurveIndex(static_cast<uint8_t>(params.curveIndex ? *(params.curveIndex) : 0.0f));
  engine.setChaos(params.chaos ? *(params.chaos) : 0.0f);
  engine.setEnvToDriveDb(params.envToDriveDb ? *(params.envToDriveDb) : 0.0f);
  engine.setGateComp(params.gateComp ? *(params.gateComp) : 0.0f);
  engine.setPreTilt(params.preTiltDbPerOct ? *(params.preTiltDbPerOct) : 0.0f);
  engine.setPostAir(params.postAirDb ? *(params.postAirDb) : 0.0f);
  engine.setDirt(params.dirtAmount ? *(params.dirtAmount) : 0.0f);
  engine.setCeiling(params.ceilingDb ? *(params.ceilingDb) : -1.0f);
  engine.setOutputTrimDb(params.outputTrimDb ? *(params.outputTrimDb) : 0.0f);

  const float mixPct = params.mixPercent ? *(params.mixPercent) : 100.0f;
  engine.setMix(mixPct * 0.01f);
}

void DustPressAudioProcessor::resizeScratchBuffers(int numSamples) {
  if (scratchLeft.size() < static_cast<size_t>(numSamples)) {
    scratchLeft.resize(static_cast<size_t>(numSamples));
  }
  if (scratchRight.size() < static_cast<size_t>(numSamples)) {
    scratchRight.resize(static_cast<size_t>(numSamples));
  }
}

void DustPressAudioProcessor::primeEngineForBlock(int numSamples) {
  if (getSampleRate() != lastSampleRate) {
    lastSampleRate = getSampleRate();
    engine.setSampleRate(static_cast<float>(lastSampleRate));
    engine.reset();
    refreshLatencyFromEngine();
  }

  if (numSamples != lastBlockSize || scratchLeft.size() < static_cast<std::size_t>(numSamples) ||
      scratchRight.size() < static_cast<std::size_t>(numSamples)) {
    lastBlockSize = numSamples;
    resizeScratchBuffers(numSamples);
  }

  syncParametersToEngine();
}

