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

  float sampleRate = 44100.0f;
  float ceilingDb = -1.0f;
  float lookaheadMs = 0.5f;
  float ceilingLinear = 0.89125094f; // -1 dBFS default
  std::vector<float> delay;
  std::size_t delayIndex = 0;
  SoftSaturation sat;
};
