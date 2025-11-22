#pragma once

#include <cmath>

// Minimal tilt EQ: split into low/high via one-pole and rebalance.
class TiltEQ {
public:
  void setSampleRate(float sr) { sampleRate = sr; updateCoeff(); }
  void setSlope(float dbPerOct) { slope = dbPerOct; }

  float process(float x) {
    // simple first order tilt: lowpass + highpass difference
    low += coeff * (x - low);
    const float high = x - low;
    const float tilt = slope * 0.05f; // modest scaling to keep sane
    return x + tilt * (high - low);
  }

  void reset(float value = 0.0f) { low = value; }

private:
  void updateCoeff() { coeff = std::exp(-2.0f * 3.14159265f * 200.0f / sampleRate); }

  float sampleRate = 44100.0f;
  float slope = 0.0f;
  float coeff = 0.9f;
  float low = 0.0f;
};
