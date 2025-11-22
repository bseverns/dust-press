#include "AirEQ.h"

#include <cmath>

void AirEQ::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeff();
}

void AirEQ::setGainDb(float g) { gainDb = g; }

float AirEQ::process(float x) {
  // crude high-pass emphasis
  low += coeff * (x - low);
  const float high = x - low;
  const float gain = std::pow(10.0f, gainDb / 20.0f);
  return x + high * (gain - 1.0f);
}

void AirEQ::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void AirEQ::reset(float value) { low = value; }

void AirEQ::updateCoeff() {
  coeff = std::exp(-2.0f * 3.14159265f * 8000.0f / sampleRate);
}
