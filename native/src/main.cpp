#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

#include "NativeDustPress.h"
#include "WavFile.h"

struct CliArgs {
  std::string inputPath;
  std::string outputPath;
  float driveDb = 0.0f;
  float bias = 0.0f;
  uint8_t curveIndex = 0;
  float chaos = 0.0f;
  float envToDriveDb = 0.0f;
  float gateComp = 0.0f;
  float preTiltDbPerOct = 0.0f;
  float postAirDb = 0.0f;
  float dirt = 0.0f;
  float ceilingDb = -1.0f;
  float outputTrimDb = 0.0f;
  float mix = 1.0f;
};

void printUsage(){
  std::cout << "dustpress_cli --in input.wav --out output.wav [options]\n";
  std::cout << "Options (mirror the Teensy controls):\n";
  std::cout << "  --drive-db <dB>           Drive before the shaper (default 0)\n";
  std::cout << "  --bias <value>            DC bias into the curve (default 0)\n";
  std::cout << "  --curve <index>           Curve index 0..n (default 0)\n";
  std::cout << "  --chaos <0-1>             Crackle/chaos amount (default 0)\n";
  std::cout << "  --env-to-drive-db <dB>    Envelope to drive modulation (default 0)\n";
  std::cout << "  --gate-comp <0-1>         Envelope gate/comp amount (default 0)\n";
  std::cout << "  --pre-tilt <dB/oct>       Tilt EQ pre-shaper (default 0)\n";
  std::cout << "  --post-air <dB>           Air shelf post-shaper (default 0)\n";
  std::cout << "  --dirt <0-1>              Curve dirt amount (default 0)\n";
  std::cout << "  --ceiling-db <dBFS>       Limiter ceiling (default -1)\n";
  std::cout << "  --output-trim-db <dB>     Output trim after mix (default 0)\n";
  std::cout << "  --mix <0-1>               Wet/dry mix (default 1)\n";
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
    if(it != kv.end()) target = std::stof(it->second);
  };

  if(kv.count("--in")) args.inputPath = kv["--in"];
  if(kv.count("--out")) args.outputPath = kv["--out"];

  getFloat("--drive-db", args.driveDb);
  getFloat("--bias", args.bias);
  getFloat("--chaos", args.chaos);
  getFloat("--env-to-drive-db", args.envToDriveDb);
  getFloat("--gate-comp", args.gateComp);
  getFloat("--pre-tilt", args.preTiltDbPerOct);
  getFloat("--post-air", args.postAirDb);
  getFloat("--dirt", args.dirt);
  getFloat("--ceiling-db", args.ceilingDb);
  getFloat("--output-trim-db", args.outputTrimDb);
  getFloat("--mix", args.mix);

  if(kv.count("--curve")) args.curveIndex = static_cast<uint8_t>(std::stoi(kv["--curve"]));

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
  processor.setDriveDb(args.driveDb);
  processor.setBias(args.bias);
  processor.setCurveIndex(args.curveIndex);
  processor.setChaos(args.chaos);
  processor.setEnvToDriveDb(args.envToDriveDb);
  processor.setGateComp(args.gateComp);
  processor.setPreTilt(args.preTiltDbPerOct);
  processor.setPostAir(args.postAirDb);
  processor.setDirt(args.dirt);
  processor.setCeiling(args.ceilingDb);
  processor.setOutputTrimDb(args.outputTrimDb);
  processor.setMix(args.mix);

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

