#pragma once

#include <cmath>

// A light-weight envelope follower for control modulation.
// Attack/release are intentionally simple to keep the code size tiny for
// microcontrollers while still giving us a usable contour.
class EnvelopeFollower {
public:
  EnvelopeFollower() { reset(); }

  void setSampleRate(float sr) { sampleRate = sr; updateCoeffs(); }
  void setAttackMs(float ms) { attackMs = ms; updateCoeffs(); }
  void setReleaseMs(float ms) { releaseMs = ms; updateCoeffs(); }

  float process(float input) {
    const float rectified = std::fabs(input);
    const float coeff = rectified > state ? attackCoeff : releaseCoeff;
    state = coeff * state + (1.0f - coeff) * rectified;
    return state;
  }

  void reset(float value = 0.0f) { state = value; updateCoeffs(); }

private:
  void updateCoeffs() {
    const float attackSamples = (attackMs * 0.001f) * sampleRate;
    const float releaseSamples = (releaseMs * 0.001f) * sampleRate;
    attackCoeff = attackSamples > 1.0f ? std::exp(-1.0f / attackSamples) : 0.0f;
    releaseCoeff = releaseSamples > 1.0f ? std::exp(-1.0f / releaseSamples) : 0.0f;
  }

  float sampleRate = 44100.0f;
  float attackMs = 5.0f;
  float releaseMs = 50.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
  float state = 0.0f;
};
