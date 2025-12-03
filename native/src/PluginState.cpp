#include "PluginState.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "NativeDustPress.h"

namespace {
template <typename T>
T clampValue(T v, T minV, T maxV) {
  return std::max(minV, std::min(v, maxV));
}

ProcessorState stateFromJsonObject(const nlohmann::json& obj,
                                   const ProcessorState& defaults) {
  ProcessorState state = defaults;
  state.name = obj.value("name", state.name);
  state.driveDb =
      clampValue<float>(obj.value("drive_db", state.driveDb), 0.0f, 36.0f);
  state.bias = clampValue<float>(obj.value("bias", state.bias), -1.0f, 1.0f);
  state.curveIndex =
      clampValue<int>(obj.value("curve", state.curveIndex), 0, 3);
  state.chaos = clampValue<float>(obj.value("chaos", state.chaos), 0.0f, 7.0f);
  state.envToDriveDb = clampValue<float>(
      obj.value("env_to_drive_db", state.envToDriveDb), -12.0f, 12.0f);
  state.gateComp =
      clampValue<float>(obj.value("gate_comp", state.gateComp), 0.0f, 1.0f);
  state.preTiltDbPerOct = clampValue<float>(
      obj.value("pre_tilt_db_per_oct", state.preTiltDbPerOct), -6.0f, 6.0f);
  state.postAirDb =
      clampValue<float>(obj.value("post_air_db", state.postAirDb), -6.0f, 6.0f);
  state.dirt = clampValue<float>(obj.value("dirt", state.dirt), 0.0f, 1.0f);
  state.ceilingDb = clampValue<float>(
      obj.value("limiter_ceiling_db", state.ceilingDb), -6.0f, 0.0f);
  state.outputTrimDb = clampValue<float>(
      obj.value("out_trim_db", state.outputTrimDb), -12.0f, 6.0f);
  state.mix = clampValue<float>(obj.value("mix", state.mix), 0.0f, 1.0f);
  return state;
}
}  // namespace

void ProcessorState::apply(NativeDustPress& engine) const {
  engine.setDriveDb(clampValue<float>(driveDb, 0.0f, 36.0f));
  engine.setBias(clampValue<float>(bias, -1.0f, 1.0f));
  engine.setCurveIndex(static_cast<uint8_t>(clampValue<int>(curveIndex, 0, 3)));
  engine.setChaos(clampValue<float>(chaos, 0.0f, 7.0f));
  engine.setEnvToDriveDb(clampValue<float>(envToDriveDb, -12.0f, 12.0f));
  engine.setGateComp(clampValue<float>(gateComp, 0.0f, 1.0f));
  engine.setPreTilt(clampValue<float>(preTiltDbPerOct, -6.0f, 6.0f));
  engine.setPostAir(clampValue<float>(postAirDb, -6.0f, 6.0f));
  engine.setDirt(clampValue<float>(dirt, 0.0f, 1.0f));
  engine.setCeiling(clampValue<float>(ceilingDb, -6.0f, 0.0f));
  engine.setOutputTrimDb(clampValue<float>(outputTrimDb, -12.0f, 6.0f));
  engine.setMix(clampValue<float>(mix, 0.0f, 1.0f));
}

PresetData ProcessorState::toTeensyPreset() const {
  PresetData preset{};
  preset.name = name.empty() ? std::string{"Untitled"} : name;
  preset.curve = clampValue<int>(curveIndex, 0, 3);
  preset.driveDb = clampValue<float>(driveDb, 0.0f, 36.0f);
  preset.bias = clampValue<float>(bias, -1.0f, 1.0f);
  preset.envToDriveDb = clampValue<float>(envToDriveDb, -12.0f, 12.0f);
  preset.mix = clampValue<float>(mix, 0.0f, 1.0f);
  preset.chaos = clampValue<int>(static_cast<int>(chaos), 0, 7);
  return preset;
}

std::vector<uint8_t> PluginStateSerializer::toChunk(const ProcessorState& state) {
  const std::string json = toJsonString(state);
  return std::vector<uint8_t>(json.begin(), json.end());
}

ProcessorState PluginStateSerializer::fromChunk(
    const std::vector<uint8_t>& chunk) {
  const std::string json(chunk.begin(), chunk.end());
  return fromJsonString(json);
}

std::string PluginStateSerializer::toJsonString(const ProcessorState& state) {
  nlohmann::json obj;
  obj["name"] = state.name;
  obj["drive_db"] = clampValue<float>(state.driveDb, 0.0f, 36.0f);
  obj["bias"] = clampValue<float>(state.bias, -1.0f, 1.0f);
  obj["curve"] = clampValue<int>(state.curveIndex, 0, 3);
  obj["chaos"] = clampValue<float>(state.chaos, 0.0f, 7.0f);
  obj["env_to_drive_db"] = clampValue<float>(state.envToDriveDb, -12.0f, 12.0f);
  obj["gate_comp"] = clampValue<float>(state.gateComp, 0.0f, 1.0f);
  obj["pre_tilt_db_per_oct"] =
      clampValue<float>(state.preTiltDbPerOct, -6.0f, 6.0f);
  obj["post_air_db"] = clampValue<float>(state.postAirDb, -6.0f, 6.0f);
  obj["dirt"] = clampValue<float>(state.dirt, 0.0f, 1.0f);
  obj["limiter_ceiling_db"] =
      clampValue<float>(state.ceilingDb, -6.0f, 0.0f);
  obj["out_trim_db"] = clampValue<float>(state.outputTrimDb, -12.0f, 6.0f);
  obj["mix"] = clampValue<float>(state.mix, 0.0f, 1.0f);
  return obj.dump();
}

ProcessorState PluginStateSerializer::fromJsonString(const std::string& json) {
  nlohmann::json obj;
  try {
    obj = nlohmann::json::parse(json);
  } catch (...) {
    return ProcessorState();
  }
  if (!obj.is_object()) {
    return ProcessorState();
  }
  return stateFromJsonObject(obj, ProcessorState());
}

ProcessorState PluginStateSerializer::fromPreset(const PresetData& preset,
                                                 const ProcessorState& defaults) {
  nlohmann::json obj;
  obj["name"] = preset.name;
  obj["drive_db"] = preset.driveDb;
  obj["bias"] = preset.bias;
  obj["curve"] = preset.curve;
  obj["chaos"] = preset.chaos;
  obj["env_to_drive_db"] = preset.envToDriveDb;
  obj["mix"] = preset.mix;
  return stateFromJsonObject(obj, defaults);
}

