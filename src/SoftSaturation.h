#pragma once

#include <cmath>

// Utility helper for gentle saturation; used by Limiter as a last-resort.
class SoftSaturation {
public:
  float process(float x) const {
    return std::tanh(x);
  }
};
