#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct StereoBuffer {
  uint32_t sampleRate = 48000;
  std::vector<float> left;
  std::vector<float> right;
};

StereoBuffer loadWavStereo(const std::string& path);
void writeWavStereo(const std::string& path, const StereoBuffer& buffer);

