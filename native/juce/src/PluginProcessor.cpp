#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
constexpr auto paramIdDrive = "drive";
constexpr auto paramIdBias = "bias";
constexpr auto paramIdCurve = "curve_index";
constexpr auto paramIdChaos = "chaos_level";
constexpr auto paramIdEnvDrive = "env_to_drive";
constexpr auto paramIdGateComp = "gate_comp";
constexpr auto paramIdPreTilt = "pre_tilt_db_per_oct";
constexpr auto paramIdPostAir = "post_air_gain_db";
constexpr auto paramIdMix = "mix";
constexpr auto paramIdDirt = "dirt_amount";
constexpr auto paramIdCeiling = "limiter_ceiling_db";
constexpr auto paramIdOutput = "out_trim_db";

constexpr float defaultDriveDb = 12.0f;
constexpr float defaultBias = 0.0f;
constexpr float defaultEnvDriveDb = 6.0f;
constexpr float defaultGateComp = 0.2f;
constexpr float defaultPreTilt = 0.0f;
constexpr float defaultPostAir = 0.0f;
constexpr float defaultMix = 0.5f;
constexpr int defaultCurve = 0;
constexpr int defaultChaos = 0;
constexpr float defaultDirt = 0.1f;
constexpr float defaultCeiling = -1.0f;
constexpr float defaultOutput = 0.0f;
} // namespace

DustPressAudioProcessor::DustPressAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DustPressParams", createParameterLayout()) {}

void DustPressAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  processor.setSampleRate(static_cast<float>(sampleRate));
  processor.reset();
}

void DustPressAudioProcessor::releaseResources() {}

bool DustPressAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
  return true;
}

void DustPressAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  const auto driveDb = parameters.getRawParameterValue(paramIdDrive)->load();
  const auto bias = parameters.getRawParameterValue(paramIdBias)->load();
  const auto curveIdx = static_cast<uint8_t>(parameters.getRawParameterValue(paramIdCurve)->load());
  const auto chaos = parameters.getRawParameterValue(paramIdChaos)->load();
  const auto envToDriveDb = parameters.getRawParameterValue(paramIdEnvDrive)->load();
  const auto gateComp = parameters.getRawParameterValue(paramIdGateComp)->load();
  const auto preTilt = parameters.getRawParameterValue(paramIdPreTilt)->load();
  const auto postAir = parameters.getRawParameterValue(paramIdPostAir)->load();
  const auto mix = parameters.getRawParameterValue(paramIdMix)->load();
  const auto dirt = parameters.getRawParameterValue(paramIdDirt)->load();
  const auto ceiling = parameters.getRawParameterValue(paramIdCeiling)->load();
  const auto output = parameters.getRawParameterValue(paramIdOutput)->load();

  processor.setDriveDb(driveDb);
  processor.setBias(bias);
  processor.setCurveIndex(curveIdx);
  processor.setChaos(chaos);
  processor.setEnvToDriveDb(envToDriveDb);
  processor.setGateComp(gateComp);
  processor.setPreTilt(preTilt);
  processor.setPostAir(postAir);
  processor.setMix(mix);
  processor.setDirt(dirt);
  processor.setCeiling(ceiling);
  processor.setOutputTrimDb(output);

  const auto numSamples = static_cast<std::size_t>(buffer.getNumSamples());
  processor.processBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getWritePointer(0),
                         buffer.getWritePointer(1), numSamples);
}

juce::AudioProcessorEditor* DustPressAudioProcessor::createEditor() {
  return new DustPressAudioProcessorEditor(*this);
}

void DustPressAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::MemoryOutputStream(destData, true).writeValueTree(parameters.copyState());
}

void DustPressAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
  if (tree.isValid()) parameters.replaceState(tree);
}

juce::AudioProcessorValueTreeState::ParameterLayout DustPressAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdDrive, "Drive", juce::NormalisableRange<float>(0.0f, 36.0f, 0.01f, 0.35f), defaultDriveDb,
      juce::AudioParameterFloatAttributes().withLabel("dB")));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdBias, "Bias", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), defaultBias));

  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      paramIdCurve, "Curve", juce::StringArray{"Tanh", "Cubic", "Diode", "Fold"}, defaultCurve));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdEnvDrive, "Env→Drive", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), defaultEnvDriveDb,
      juce::AudioParameterFloatAttributes().withLabel("dB")));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdGateComp, "GateComp", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), defaultGateComp));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdPreTilt, "Pre Tilt", juce::NormalisableRange<float>(-6.0f, 6.0f, 0.01f), defaultPreTilt,
      juce::AudioParameterFloatAttributes().withLabel("dB/oct")));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdPostAir, "Post Air", juce::NormalisableRange<float>(-6.0f, 6.0f, 0.01f), defaultPostAir,
      juce::AudioParameterFloatAttributes().withLabel("dB")));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdMix, "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), defaultMix));

  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      paramIdChaos, "Chaos", juce::StringArray{"0", "1", "2", "3", "4", "5", "6", "7"}, defaultChaos));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdDirt, "Dirt", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), defaultDirt));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdCeiling, "Ceiling", juce::NormalisableRange<float>(-6.0f, 0.0f, 0.01f), defaultCeiling,
      juce::AudioParameterFloatAttributes().withLabel("dBFS")));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      paramIdOutput, "Output", juce::NormalisableRange<float>(-12.0f, 6.0f, 0.01f), defaultOutput,
      juce::AudioParameterFloatAttributes().withLabel("dB")));

  return {params.begin(), params.end()};
}
