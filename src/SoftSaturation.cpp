#include "SoftSaturation.h"

#include <cmath>

float SoftSaturation::process(float x) const { return std::tanh(x); }

void SoftSaturation::processBlock(float* buffer, std::size_t count) const {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}
