#pragma once

#include <cstddef>

// Minimal tilt EQ: split into low/high via one-pole and rebalance.
class TiltEQ {
public:
  TiltEQ();

  void setSampleRate(float sr);
  void setSlope(float dbPerOct);

  float process(float x);
  void processBlock(float* buffer, std::size_t count);

  void reset(float value = 0.0f);

private:
  void updateCoeff();
  void updateGains();

  float sampleRate = 44100.0f;
  float slope = 0.0f;
  float coeff = 0.9f;
  float lowGain = 1.0f;
  float highGain = 1.0f;
  float low = 0.0f;
};
