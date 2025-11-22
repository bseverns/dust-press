#include "TiltEQ.h"

#include <cmath>

void TiltEQ::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeff();
}

void TiltEQ::setSlope(float dbPerOct) { slope = dbPerOct; }

float TiltEQ::process(float x) {
  // simple first order tilt: lowpass + highpass difference
  low += coeff * (x - low);
  const float high = x - low;
  const float tilt = slope * 0.05f; // modest scaling to keep sane
  return x + tilt * (high - low);
}

void TiltEQ::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void TiltEQ::reset(float value) { low = value; }

void TiltEQ::updateCoeff() {
  coeff = std::exp(-2.0f * 3.14159265f * 200.0f / sampleRate);
}
