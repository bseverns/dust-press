#pragma once

#include <cmath>

// Quick one-pole smoother for parameter modulation.
class ParamSmoother {
public:
  void setSampleRate(float sr) { sampleRate = sr; updateCoeff(); }
  void setTimeMs(float ms) { timeMs = ms; updateCoeff(); }

  void setTarget(float t) { target = t; }
  float process() {
    state = coeff * state + (1.0f - coeff) * target;
    return state;
  }

  float getCurrent() const { return state; }
  void reset(float value = 0.0f) { state = target = value; }

private:
  void updateCoeff() {
    const float samples = (timeMs * 0.001f) * sampleRate;
    coeff = samples > 1.0f ? std::exp(-1.0f / samples) : 0.0f;
  }

  float sampleRate = 44100.0f;
  float timeMs = 10.0f;
  float coeff = 0.0f;
  float state = 0.0f;
  float target = 0.0f;
};
