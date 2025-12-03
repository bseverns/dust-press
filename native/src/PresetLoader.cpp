#include "PresetLoader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "NativeDustPress.h"

namespace {
template <typename T>
T clampValue(T v, T minV, T maxV) {
  return std::max(minV, std::min(v, maxV));
}

PresetData presetFromJson(const nlohmann::json& obj) {
  PresetData preset{};

  preset.name = obj.value("name", std::string{"Untitled"});
  preset.curve = clampValue<int>(obj.value("curve", preset.curve), 0, 3);
  preset.driveDb =
      clampValue<float>(obj.value("drive_db", preset.driveDb), 0.0f, 36.0f);
  preset.bias = clampValue<float>(obj.value("bias", preset.bias), -1.0f, 1.0f);
  preset.envToDriveDb = clampValue<float>(
      obj.value("env_to_drive_db", preset.envToDriveDb), -12.0f, 12.0f);
  preset.mix = clampValue<float>(obj.value("mix", preset.mix), 0.0f, 1.0f);
  preset.chaos = clampValue<int>(obj.value("chaos", preset.chaos), 0, 7);

  return preset;
}
}  // namespace

std::vector<PresetData> loadPresetsFromFile(const std::filesystem::path& jsonPath) {
  std::ifstream stream(jsonPath);
  if (!stream.is_open()) {
    std::cerr << "Failed to open preset file: " << jsonPath << "\n";
    return {};
  }

  nlohmann::json doc;
  try {
    stream >> doc;
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse preset file: " << e.what() << "\n";
    return {};
  }

  if (!doc.is_array()) {
    std::cerr << "Preset file must contain a top-level array." << "\n";
    return {};
  }

  std::vector<PresetData> presets;
  presets.reserve(doc.size());
  for (const auto& entry : doc) {
    if (!entry.is_object()) {
      continue;
    }
    presets.push_back(presetFromJson(entry));
  }

  return presets;
}

void applyPresetToEngine(const PresetData& preset, NativeDustPress& engine) {
  engine.setCurveIndex(static_cast<uint8_t>(clampValue<int>(preset.curve, 0, 3)));
  engine.setDriveDb(clampValue<float>(preset.driveDb, 0.0f, 36.0f));
  engine.setBias(clampValue<float>(preset.bias, -1.0f, 1.0f));
  engine.setEnvToDriveDb(clampValue<float>(preset.envToDriveDb, -12.0f, 12.0f));
  engine.setMix(clampValue<float>(preset.mix, 0.0f, 1.0f));
  engine.setChaos(static_cast<float>(clampValue<int>(preset.chaos, 0, 7)));
}

const PresetData* findPresetByName(const std::vector<PresetData>& presets,
                                  const std::string& name) {
  for (const auto& preset : presets) {
    if (preset.name == name) {
      return &preset;
    }
  }
  return nullptr;
}

const PresetData* findPresetByIndex(const std::vector<PresetData>& presets,
                                   std::size_t index) {
  if (index < presets.size()) {
    return &presets[index];
  }
  return nullptr;
}

