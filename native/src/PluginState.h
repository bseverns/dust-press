#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "PresetLoader.h"

class NativeDustPress;

struct ProcessorState {
  std::string name;
  float driveDb = 12.0f;
  float bias = 0.0f;
  int curveIndex = 0;
  float chaos = 0.0f;
  float envToDriveDb = 6.0f;
  float gateComp = 0.2f;
  float preTiltDbPerOct = 0.0f;
  float postAirDb = 0.0f;
  float dirt = 0.1f;
  float ceilingDb = -1.0f;
  float outputTrimDb = 0.0f;
  float mix = 0.5f;

  void apply(NativeDustPress& engine) const;
  PresetData toTeensyPreset() const;
};

// Helper for a JUCE ValueTree or VST3 chunk state. Serializes every parameter
// so plugin state recall stays in sync with firmware presets.
class PluginStateSerializer {
 public:
  static std::vector<uint8_t> toChunk(const ProcessorState& state);
  static ProcessorState fromChunk(const std::vector<uint8_t>& chunk);

  static std::string toJsonString(const ProcessorState& state);
  static ProcessorState fromJsonString(const std::string& json);

  static ProcessorState fromPreset(const PresetData& preset,
                                   const ProcessorState& defaults =
                                       ProcessorState());
};

