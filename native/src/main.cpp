#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "NativeDustPress.h"
#include "WavFile.h"
#include <nlohmann/json.hpp>
#include "PluginState.h"
#include "PresetLoader.h"

struct CliArgs {
  std::string inputPath;
  std::string outputPath;
  std::optional<float> driveDb;
  std::optional<float> bias;
  std::optional<int> curveIndex;
  std::optional<float> chaos;
  std::optional<float> envToDriveDb;
  std::optional<float> gateComp;
  std::optional<float> preTiltDbPerOct;
  std::optional<float> postAirDb;
  std::optional<float> dirt;
  std::optional<float> ceilingDb;
  std::optional<float> outputTrimDb;
  std::optional<float> mix;
  std::optional<std::string> presetName;
  std::optional<int> presetIndex;
  std::optional<std::string> presetPath;
  std::optional<std::string> exportStatePath;
  std::optional<std::string> exportPresetPath;
};

void printUsage(){
  std::cout << "dustpress_cli --in input.wav --out output.wav [options]\n";
  std::cout << "Options (mirror the Teensy controls):\n";
  std::cout << "  --drive-db <dB>           Drive before the shaper (default 12)\n";
  std::cout << "  --bias <value>            DC bias into the curve (default 0)\n";
  std::cout << "  --curve <index>           Curve index 0..n (default 0)\n";
  std::cout << "  --chaos <0-1>             Crackle/chaos amount (default 0)\n";
  std::cout << "  --env-to-drive-db <dB>    Envelope to drive modulation (default +6)\n";
  std::cout << "  --gate-comp <0-1>         Envelope gate/comp amount (default 0.2)\n";
  std::cout << "  --pre-tilt <dB/oct>       Tilt EQ pre-shaper (default 0)\n";
  std::cout << "  --post-air <dB>           Air shelf post-shaper (default 0)\n";
  std::cout << "  --dirt <0-1>              Curve dirt amount (default 0.1)\n";
  std::cout << "  --ceiling-db <dBFS>       Limiter ceiling (default -1)\n";
  std::cout << "  --output-trim-db <dB>     Output trim after mix (default 0)\n";
  std::cout << "  --mix <0-1>               Wet/dry mix (default 0.5)\n";
  std::cout << "  --preset <name>           Load preset name from presets/presets.json\n";
  std::cout << "  --preset-index <n>        Load preset N (0-based) from presets/presets.json\n";
  std::cout << "  --preset-path <file>      Override default preset path\n";
  std::cout << "  --save-state <file>       Export full plugin-style state JSON\n";
  std::cout << "  --save-preset <file>      Export Teensy-compatible preset JSON object\n";
}

CliArgs parseArgs(int argc, char** argv){
  CliArgs args{};
  std::unordered_map<std::string, std::string> kv;

  for(int i=1;i<argc;i++){
    const std::string key = argv[i];
    if(key == "--help"){ printUsage(); std::exit(0); }
    if(i + 1 >= argc){
      std::cerr << "Missing value for argument: " << key << "\n";
      printUsage();
      std::exit(1);
    }
    kv[key] = argv[++i];
  }

  auto getFloat = [&kv](const std::string& k, float& target){
    auto it = kv.find(k);
    if(it != kv.end()) {
      target = std::stof(it->second);
      return true;
    }
    return false;
  };

  if(kv.count("--in")) args.inputPath = kv["--in"];
  if(kv.count("--out")) args.outputPath = kv["--out"];

  float value = 0.0f;
  if(getFloat("--drive-db", value)) args.driveDb = value;
  if(getFloat("--bias", value)) args.bias = value;
  if(getFloat("--chaos", value)) args.chaos = value;
  if(getFloat("--env-to-drive-db", value)) args.envToDriveDb = value;
  if(getFloat("--gate-comp", value)) args.gateComp = value;
  if(getFloat("--pre-tilt", value)) args.preTiltDbPerOct = value;
  if(getFloat("--post-air", value)) args.postAirDb = value;
  if(getFloat("--dirt", value)) args.dirt = value;
  if(getFloat("--ceiling-db", value)) args.ceilingDb = value;
  if(getFloat("--output-trim-db", value)) args.outputTrimDb = value;
  if(getFloat("--mix", value)) args.mix = value;

  if(kv.count("--curve")) args.curveIndex = static_cast<int>(std::stoi(kv["--curve"]));

  if(kv.count("--preset")) args.presetName = kv["--preset"];
  if(kv.count("--preset-index")) args.presetIndex = std::stoi(kv["--preset-index"]);
  if(kv.count("--preset-path")) args.presetPath = kv["--preset-path"];
  if(kv.count("--save-state")) args.exportStatePath = kv["--save-state"];
  if(kv.count("--save-preset")) args.exportPresetPath = kv["--save-preset"];

  if(args.inputPath.empty() || args.outputPath.empty()){
    std::cerr << "--in and --out are required.\n";
    printUsage();
    std::exit(1);
  }

  return args;
}

int main(int argc, char** argv){
  const CliArgs args = parseArgs(argc, argv);

  StereoBuffer input = loadWavStereo(args.inputPath);

  NativeDustPress processor(static_cast<float>(input.sampleRate));

  ProcessorState state{};
  const std::filesystem::path presetPath =
      args.presetPath.value_or(std::string("presets/presets.json"));

  if (args.presetName || args.presetIndex) {
    const std::vector<PresetData> presets = loadPresetsFromFile(presetPath);
    const PresetData* selected = nullptr;
    if (args.presetName) {
      selected = findPresetByName(presets, *args.presetName);
      if (!selected) {
        std::cerr << "Preset name not found: " << *args.presetName << "\n";
      }
    }
    if (!selected && args.presetIndex) {
      selected = findPresetByIndex(presets, static_cast<std::size_t>(*args.presetIndex));
      if (!selected) {
        std::cerr << "Preset index out of range: " << *args.presetIndex << "\n";
      }
    }
    if (selected) {
      state = PluginStateSerializer::fromPreset(*selected, state);
    }
  }

  auto applyOptional = [](const std::optional<float>& source, float& target,
                          float minV, float maxV) {
    if (source) {
      target = std::clamp(*source, minV, maxV);
    }
  };

  if (args.curveIndex)
    state.curveIndex = std::clamp(*args.curveIndex, 0, 3);
  applyOptional(args.driveDb, state.driveDb, 0.0f, 36.0f);
  applyOptional(args.bias, state.bias, -1.0f, 1.0f);
  applyOptional(args.chaos, state.chaos, 0.0f, 7.0f);
  applyOptional(args.envToDriveDb, state.envToDriveDb, -12.0f, 12.0f);
  applyOptional(args.gateComp, state.gateComp, 0.0f, 1.0f);
  applyOptional(args.preTiltDbPerOct, state.preTiltDbPerOct, -6.0f, 6.0f);
  applyOptional(args.postAirDb, state.postAirDb, -6.0f, 6.0f);
  applyOptional(args.dirt, state.dirt, 0.0f, 1.0f);
  applyOptional(args.ceilingDb, state.ceilingDb, -6.0f, 0.0f);
  applyOptional(args.outputTrimDb, state.outputTrimDb, -12.0f, 6.0f);
  applyOptional(args.mix, state.mix, 0.0f, 1.0f);

  state.apply(processor);

  auto writeTextFile = [](const std::filesystem::path& path,
                          const std::string& content) {
    std::ofstream out(path);
    if (!out.is_open()) {
      std::cerr << "Failed to write file: " << path << "\n";
      return;
    }
    out << content;
  };

  if (args.exportStatePath) {
    writeTextFile(*args.exportStatePath,
                 PluginStateSerializer::toJsonString(state));
  }

  if (args.exportPresetPath) {
    const PresetData teensyPreset = state.toTeensyPreset();
    nlohmann::json presetJson;
    presetJson["name"] = teensyPreset.name;
    presetJson["curve"] = teensyPreset.curve;
    presetJson["drive_db"] = teensyPreset.driveDb;
    presetJson["bias"] = teensyPreset.bias;
    presetJson["env_to_drive_db"] = teensyPreset.envToDriveDb;
    presetJson["mix"] = teensyPreset.mix;
    presetJson["chaos"] = teensyPreset.chaos;
    writeTextFile(*args.exportPresetPath, presetJson.dump());
  }

  StereoBuffer output{};
  output.sampleRate = input.sampleRate;
  output.left.resize(input.left.size());
  output.right.resize(input.right.size());

  processor.processBlock(input.left.data(),
                         input.right.data(),
                         output.left.data(),
                         output.right.data(),
                         input.left.size());

  writeWavStereo(args.outputPath, output);

  std::cout << "Processed " << input.left.size() << " frames at " << input.sampleRate
            << " Hz -> " << args.outputPath << "\n";

  return 0;
}

