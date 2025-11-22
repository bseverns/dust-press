#pragma once
#include <cmath>

// One-pole tilt EQ made from a split low-pass/high-pass blend. Positive slope
// boosts highs while gently trimming lows; negative slope does the inverse.
class TiltEQ {
public:
  TiltEQ(float sampleRate = 44100.0f, float pivotHz = 1000.0f, float slopeDbPerOct = 0.0f) {
    setSampleRate(sampleRate);
    setPivot(pivotHz);
    setSlopeDbPerOct(slopeDbPerOct);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    updateCoeffs();
  }

  void setPivot(float hz) {
    pivotHz = hz;
    updateCoeffs();
  }

  void setSlopeDbPerOct(float slope) {
    slopeDbPerOct = slope;
    updateGains();
  }

  float processSample(float x) {
    // Split signal
    lowState = lowState + lowCoeff * (x - lowState);
    const float high = x - lowState;
    const float y = lowState * lowGain + high * highGain;
    return y;
  }

  void reset(float value = 0.0f) { lowState = value; }

private:
  void updateCoeffs() {
    const float omega = 2.0f * static_cast<float>(M_PI) * pivotHz / sampleRate;
    // Simple one-pole smoothing coefficient.
    lowCoeff = 1.0f - std::exp(-omega);
  }

  void updateGains() {
    // Approximate tilt: apply opposing shelves of equal magnitude.
    const float gain = std::pow(10.0f, slopeDbPerOct / 20.0f);
    highGain = gain;
    lowGain = 1.0f / gain;
  }

  float sampleRate = 44100.0f;
  float pivotHz = 1000.0f;
  float slopeDbPerOct = 0.0f;
  float lowCoeff = 0.0f;
  float lowState = 0.0f;
  float lowGain = 1.0f;
  float highGain = 1.0f;
};
