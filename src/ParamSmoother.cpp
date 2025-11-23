#include "ParamSmoother.h"

#include <cmath>

ParamSmoother::ParamSmoother() { updateCoeff(); }

void ParamSmoother::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeff();
}

void ParamSmoother::setTimeMs(float ms) {
  timeMs = ms;
  updateCoeff();
}

void ParamSmoother::setTarget(float t) { target = t; }

float ParamSmoother::process() {
  state = coeff * state + (1.0f - coeff) * target;
  return state;
}

void ParamSmoother::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process();
  }
}

float ParamSmoother::getCurrent() const { return state; }

void ParamSmoother::reset(float value) { state = target = value; }

void ParamSmoother::updateCoeff() {
  const float samples = (timeMs * 0.001f) * sampleRate;
  coeff = samples > 1.0f ? std::exp(-1.0f / samples) : 0.0f;
}
