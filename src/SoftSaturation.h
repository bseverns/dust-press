#pragma once

#include <cstddef>

// Utility helper for gentle saturation; used by Limiter as a last-resort.
class SoftSaturation {
public:
  SoftSaturation() = default;

  float process(float x) const;
  void processBlock(float* buffer, std::size_t count) const;
};
