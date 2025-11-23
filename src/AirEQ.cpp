#include "AirEQ.h"

#include <cmath>

AirEQ::AirEQ() { updateCoeff(); }

void AirEQ::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeff();
}

void AirEQ::setGainDb(float g) {
  gainDb = g;
  airGain = std::pow(10.0f, gainDb / 20.0f);
}

float AirEQ::process(float x) {
  // One-pole high shelf anchored around 10 kHz.
  low += coeff * (x - low);
  const float high = x - low;
  return low + high * airGain;
}

void AirEQ::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void AirEQ::reset(float value) {
  low = value;
}

void AirEQ::updateCoeff() {
  const float cutoff = 10000.0f;
  coeff = std::exp(-2.0f * 3.14159265f * cutoff / sampleRate);
  airGain = std::pow(10.0f, gainDb / 20.0f);
}
