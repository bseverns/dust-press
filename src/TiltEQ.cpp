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
  return process(x, defaultState);
}

float TiltEQ::process(float x, ChannelState& state) {
  // One-pole split.
  state.low += coeff * (x - state.low);
  const float high = x - state.low;
  return state.low * lowGain + high * highGain;
}

void TiltEQ::processBlock(float* buffer, std::size_t count) {
  processBlock(buffer, count, defaultState);
}

void TiltEQ::processBlock(float* buffer, std::size_t count, ChannelState& state) {
  for(std::size_t i = 0; i < count; ++i) { buffer[i] = process(buffer[i], state); }
}

void TiltEQ::reset(float value) {
  reset(defaultState, value);
}

void TiltEQ::reset(ChannelState& state, float value) {
  state.low = value;
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
