#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class NativeDustPress;

struct PresetData {
  std::string name;
  int curve = 0;
  float driveDb = 12.0f;
  float bias = 0.0f;
  float envToDriveDb = 6.0f;
  float mix = 0.5f;
  int chaos = 0;
};

// Loads presets/presets.json (or a custom path) and clamps per docs/USAGE.md.
// Missing fields fall back to the canonical control defaults so firmware and
// native builds stay interchangeable.
std::vector<PresetData> loadPresetsFromFile(
    const std::filesystem::path& jsonPath = std::filesystem::path("presets") /
                                            "presets.json");

// Applies a Teensy-format preset to the NativeDustPress engine, honoring the
// same clamping used at load time.
void applyPresetToEngine(const PresetData& preset, NativeDustPress& engine);

// Utility helpers for selecting presets after load.
const PresetData* findPresetByName(const std::vector<PresetData>& presets,
                                  const std::string& name);
const PresetData* findPresetByIndex(const std::vector<PresetData>& presets,
                                   std::size_t index);

