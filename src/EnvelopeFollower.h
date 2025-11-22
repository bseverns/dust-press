#pragma once
#include <algorithm>
#include <cmath>

// Simple peak-style envelope follower with independent attack and release
// coefficients. Useful for level-driven modulation and the limiter detector.
class EnvelopeFollower {
public:
  EnvelopeFollower(float sampleRate = 44100.0f, float attackMs = 5.0f, float releaseMs = 50.0f) {
    setSampleRate(sampleRate);
    setTimesMs(attackMs, releaseMs);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    updateCoeffs();
  }

  void setTimesMs(float attackMs, float releaseMs) {
    attackTimeMs = attackMs;
    releaseTimeMs = releaseMs;
    updateCoeffs();
  }

  float processSample(float x) {
    const float rectified = std::fabs(x);
    const float coeff = (rectified > envelope) ? attackCoeff : releaseCoeff;
    envelope = (1.0f - coeff) * rectified + coeff * envelope;
    return envelope;
  }

  void reset(float value = 0.0f) { envelope = value; }

  float getEnvelope() const { return envelope; }

private:
  void updateCoeffs() {
    // One-pole smoothing; clamp to avoid denormals.
    const float eps = 1e-6f;
    attackCoeff = std::exp(-1.0f / std::max(sampleRate * (attackTimeMs / 1000.0f), eps));
    releaseCoeff = std::exp(-1.0f / std::max(sampleRate * (releaseTimeMs / 1000.0f), eps));
  }

  float sampleRate = 44100.0f;
  float attackTimeMs = 5.0f;
  float releaseTimeMs = 50.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
  float envelope = 0.0f;
};
