#include "TiltEQ.h"

#include <cmath>

TiltEQ::TiltEQ() { updateCoeff(); }

void TiltEQ::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeff();
}

void TiltEQ::setSlope(float dbPerOct) {
  slope = dbPerOct;
  updateGains();
}

float TiltEQ::process(float x) {
  // One-pole split.
  low += coeff * (x - low);
  const float high = x - low;
  return low * lowGain + high * highGain;
}

void TiltEQ::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void TiltEQ::reset(float value) {
  low = value;
}

void TiltEQ::updateCoeff() {
  // Pivot around ~650 Hz for a broad tone tilt.
  const float cutoff = 650.0f;
  coeff = std::exp(-2.0f * 3.14159265f * cutoff / sampleRate);
  updateGains();
}

void TiltEQ::updateGains() {
  // Convert slope dB/oct to complementary shelf gains.
  const float gainHigh = std::pow(10.0f, slope / 20.0f);
  highGain = gainHigh;
  lowGain = 1.0f / gainHigh;
}
