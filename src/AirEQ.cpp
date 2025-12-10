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
  return process(x, defaultState);
}

float AirEQ::process(float x, ChannelState& state) {
  // One-pole high shelf anchored around 10 kHz.
  state.low += coeff * (x - state.low);
  const float high = x - state.low;
  return state.low + high * airGain;
}

void AirEQ::processBlock(float* buffer, std::size_t count) {
  processBlock(buffer, count, defaultState);
}

void AirEQ::processBlock(float* buffer, std::size_t count, ChannelState& state) {
  for(std::size_t i = 0; i < count; ++i) { buffer[i] = process(buffer[i], state); }
}

void AirEQ::reset(float value) {
  reset(defaultState, value);
}

void AirEQ::reset(ChannelState& state, float value) {
  state.low = value;
}

void AirEQ::updateCoeff() {
  const float cutoff = 10000.0f;
  coeff = std::exp(-2.0f * 3.14159265f * cutoff / sampleRate);
  airGain = std::pow(10.0f, gainDb / 20.0f);
}
