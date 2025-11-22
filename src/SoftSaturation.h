#pragma once
#include <cmath>

// Soft saturation block with drive, bias, and blend controls.
class SoftSaturation {
public:
  SoftSaturation(float driveDb = 0.0f, float bias = 0.0f, float mix = 1.0f) {
    setDriveDb(driveDb);
    setBias(bias);
    setMix(mix);
  }

  void setDriveDb(float db) { drive = std::pow(10.0f, db / 20.0f); }
  void setBias(float b) { bias = b; }
  void setMix(float m) { mix = (m < 0.0f) ? 0.0f : (m > 1.0f ? 1.0f : m); }

  float processSample(float x) const {
    const float pushed = (x + bias) * drive;
    const float shaped = std::tanh(pushed);
    return shaped * mix + x * (1.0f - mix);
  }

private:
  float drive = 1.0f;
  float bias = 0.0f;
  float mix = 1.0f;
};
