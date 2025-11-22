#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// Single-pole parameter smoother that can run per-sample or on a block.
class ParamSmoother {
public:
  ParamSmoother(float sampleRate = 44100.0f, float timeMs = 10.0f) {
    setSampleRate(sampleRate);
    setTimeMs(timeMs);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    updateCoeff();
  }

  void setTimeMs(float timeMs) {
    smoothingMs = timeMs;
    updateCoeff();
  }

  void setTarget(float t) { target = t; }

  float getCurrent() const { return current; }

  float processSample() {
    current += coeff * (target - current);
    return current;
  }

  void processBlock(float* buffer, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
      buffer[i] = processSample();
    }
  }

  void reset(float value) {
    current = target = value;
  }

private:
  void updateCoeff() {
    const float eps = 1e-6f;
    const float tau = std::max(smoothingMs / 1000.0f, eps);
    coeff = 1.0f - std::exp(-1.0f / std::max(sampleRate * tau, eps));
  }

  float sampleRate = 44100.0f;
  float smoothingMs = 10.0f;
  float coeff = 0.0f;
  float target = 0.0f;
  float current = 0.0f;
};
