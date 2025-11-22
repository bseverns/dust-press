#pragma once

#include <cstddef>
#include <vector>

#include "SoftSaturation.h"

// Super tiny lookahead limiter — enough for demos/tests.
class LimiterLookahead {
public:
  LimiterLookahead();

  void setSampleRate(float sr);
  void setCeilingDb(float db);
  void setLookaheadMs(float ms);

  float process(float x);
  void processBlock(float* buffer, std::size_t count);

  void reset();

private:
  void refreshGain();
  void updateDetectorCoeffs();

  float sampleRate = 44100.0f;
  float ceilingDb = -1.0f;
  float lookaheadMs = 0.5f;
  float ceilingLinear = 0.89125094f; // -1 dBFS default
  float attackMs = 0.1f;
  float releaseMs = 10.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
  float gainSmooth = 1.0f;
  float gainCoeff = 0.0f;
  float envelope = 0.0f;
  std::vector<float> delay;
  std::size_t delayIndex = 0;
  SoftSaturation sat;
};
