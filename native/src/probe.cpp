#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "NativeDustPress.h"

// A headless, host-ish stress probe for NativeDustPress. It streams synthetic
// audio through the processor while juggling block sizes, sample rates, and
// parameter sweeps. CSV telemetry is dumped so you can plot how the envelope
// gate and lookahead limiter behave when the buffer cadence gets messy.

struct ProbeConfig {
  float seconds = 6.0f;
  std::string outPath = "native_probe.csv";
};

struct StereoBufferF {
  std::vector<float> left;
  std::vector<float> right;
};

ProbeConfig parseArgs(int argc, char** argv) {
  ProbeConfig cfg{};
  for(int i = 1; i < argc; ++i) {
    const std::string key = argv[i];
    if(key == "--help") {
      std::cout << "dustpress_probe --seconds 6 --out native_probe.csv\n";
      std::exit(0);
    }
    if(key == "--seconds" && i + 1 < argc) {
      cfg.seconds = std::stof(argv[++i]);
    } else if(key == "--out" && i + 1 < argc) {
      cfg.outPath = argv[++i];
    }
  }
  return cfg;
}

StereoBufferF generateStimulus(float sampleRate, std::size_t frames) {
  StereoBufferF buffer{};
  buffer.left.resize(frames);
  buffer.right.resize(frames);

  std::mt19937 rng(444);
  std::uniform_real_distribution<float> noise(-0.08f, 0.08f);

  const float twoPi = 6.28318530718f;
  for(std::size_t i = 0; i < frames; ++i) {
    const float t = static_cast<float>(i) / sampleRate;
    float tone = 0.35f * std::sin(twoPi * 110.0f * t);
    tone += 0.25f * std::sin(twoPi * 880.0f * t + 0.4f);

    // Sprinkle in a nasty transient every half second to wake the limiter up.
    const std::size_t transientPeriod = static_cast<std::size_t>(sampleRate / 2);
    if(i % transientPeriod < 128) {
      tone += 0.9f * std::sin(twoPi * 40.0f * t) * (1.0f - static_cast<float>(i % transientPeriod) / 128.0f);
    }

    tone += noise(rng);

    buffer.left[i] = tone;
    buffer.right[i] = tone * 0.92f;
  }

  return buffer;
}

float sineLfo(float t, float freq, float amplitude, float offset = 0.0f) {
  const float twoPi = 6.28318530718f;
  return offset + amplitude * std::sin(twoPi * freq * t);
}

int main(int argc, char** argv) {
  const ProbeConfig cfg = parseArgs(argc, argv);
  std::ofstream csv(cfg.outPath);
  if(!csv.is_open()) {
    std::cerr << "Failed to open output: " << cfg.outPath << "\n";
    return 1;
  }

  csv << "sample_rate,sample,block,drive_db,env,gate_gain,limiter_env,limiter_gain,out_peak\n";

  const std::vector<float> sampleRates = {44100.0f, 48000.0f, 96000.0f};
  std::vector<std::size_t> blockMenu = {16, 32, 48, 64, 96, 128, 192, 256, 384};
  std::mt19937 rng(1337);
  std::uniform_int_distribution<int> jitter(-8, 12);

  for(const float sr : sampleRates) {
    const std::size_t frames = static_cast<std::size_t>(cfg.seconds * sr);
    StereoBufferF input = generateStimulus(sr, frames);
    StereoBufferF output{};
    output.left.resize(frames);
    output.right.resize(frames);

    NativeDustPress proc(sr);
    proc.setCurveIndex(2);
    proc.setPreTilt(1.5f);
    proc.setPostAir(3.0f);
    proc.setCeiling(-1.0f);
    proc.setMix(0.85f);

    std::size_t cursor = 0;
    std::size_t blockCounter = 0;
    std::vector<NativeDustPress::TelemetrySample> telemetry;

    while(cursor < frames) {
      std::size_t blockSize = blockMenu[blockCounter % blockMenu.size()];
      blockSize = static_cast<std::size_t>(std::max<int>(16, static_cast<int>(blockSize) + jitter(rng)));
      if(cursor + blockSize > frames) {
        blockSize = frames - cursor;
      }

      const float t = static_cast<float>(cursor) / sr;
      const float driveDb = sineLfo(t, 0.11f, 16.0f, -4.0f);
      const float bias = sineLfo(t, 0.31f, 0.15f);
      const float chaos = 0.25f + 0.25f * std::sin(t * 0.9f);
      const float envToDrive = sineLfo(t, 0.07f, 4.0f);
      const float gateComp = 0.35f + 0.25f * std::sin(t * 0.21f);
      const float mix = 0.7f + 0.25f * std::sin(t * 0.17f + 1.1f);

      proc.setDriveDb(driveDb);
      proc.setBias(bias);
      proc.setChaos(chaos);
      proc.setEnvToDriveDb(envToDrive);
      proc.setGateComp(gateComp);
      proc.setMix(mix);

      std::vector<float> blockL(blockSize);
      std::vector<float> blockR(blockSize);
      std::vector<float> processedL(blockSize);
      std::vector<float> processedR(blockSize);
      std::copy_n(input.left.data() + cursor, blockSize, blockL.data());
      std::copy_n(input.right.data() + cursor, blockSize, blockR.data());

      telemetry.resize(blockSize);
      proc.processBlockWithTelemetry(blockL.data(),
                                     blockR.data(),
                                     processedL.data(),
                                     processedR.data(),
                                     blockSize,
                                     telemetry.data());

      const auto logSample = [&](std::size_t localIndex) {
        const float outPeak = std::max(std::abs(processedL[localIndex]), std::abs(processedR[localIndex]));
        const std::size_t globalIndex = cursor + localIndex;
        const auto& tap = telemetry[localIndex];
        csv << static_cast<int>(sr) << ',' << globalIndex << ',' << blockSize << ','
            << tap.driveDbApplied << ',' << tap.env << ',' << tap.gateGain << ','
            << tap.limiterEnv << ',' << tap.limiterGain << ',' << outPeak << '\n';
      };

      for(std::size_t i = 0; i < blockSize; ++i) {
        logSample(i);
      }

      std::copy(processedL.begin(), processedL.end(), output.left.begin() + cursor);
      std::copy(processedR.begin(), processedR.end(), output.right.begin() + cursor);

      cursor += blockSize;
      ++blockCounter;
    }

    float peak = 0.0f;
    for(std::size_t i = 0; i < frames; ++i) {
      peak = std::max(peak, std::max(std::abs(output.left[i]), std::abs(output.right[i])));
    }

    std::cout << "[" << static_cast<int>(sr) << " Hz] "
              << blockCounter << " blocks, peak " << std::fixed << std::setprecision(3)
              << peak << " (lookahead " << proc.getLimiterLookaheadSamples() << " samples)\n";
  }

  std::cout << "Telemetry dumped to " << cfg.outPath
            << " — sling it into Python/gnuplot to visualize the envelope,\n"
            << "lookahead delay, and limiter gain curves.\n";
  return 0;
}
