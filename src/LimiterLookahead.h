#pragma once

#include <cmath>
#include "SoftSaturation.h"

// Super tiny lookahead limiter — enough for demos/tests.
class LimiterLookahead {
public:
  void setSampleRate(float sr) { sampleRate = sr; }
  void setCeilingDb(float db) { ceilingDb = db; }

  float process(float x) {
    const float ceiling = std::pow(10.0f, ceilingDb / 20.0f);
    if(std::fabs(x) <= ceiling) return x;
    // fallback to soft saturation so we do not brick-wall in a nasty way
    const float clipped = x > 0 ? ceiling : -ceiling;
    const float overflow = x - clipped;
    return sat.process(clipped + overflow * 0.25f);
  }

private:
  float sampleRate = 44100.0f;
  float ceilingDb = -1.0f;
  SoftSaturation sat;
};
