#pragma once

#include <cmath>

// Tiny high-shelf style "air" control.
class AirEQ {
public:
  void setSampleRate(float sr) { sampleRate = sr; updateCoeff(); }
  void setGainDb(float g) { gainDb = g; }

  float process(float x) {
    // crude high-pass emphasis
    low += coeff * (x - low);
    const float high = x - low;
    const float gain = std::pow(10.0f, gainDb / 20.0f);
    return x + high * (gain - 1.0f);
  }

  void reset(float value = 0.0f) { low = value; }

private:
  void updateCoeff() { coeff = std::exp(-2.0f * 3.14159265f * 8000.0f / sampleRate); }

  float sampleRate = 44100.0f;
  float gainDb = 0.0f;
  float coeff = 0.8f;
  float low = 0.0f;
};
