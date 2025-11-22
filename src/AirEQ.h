#pragma once
#include <cmath>

// First-order high-shelf used as the "air" sweetener after saturation.
class AirEQ {
public:
  AirEQ(float sampleRate = 44100.0f, float freqHz = 8000.0f, float gainDb = 0.0f) {
    setSampleRate(sampleRate);
    setFreq(freqHz);
    setGainDb(gainDb);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    updateCoeffs();
  }

  void setFreq(float hz) {
    freqHz = hz;
    updateCoeffs();
  }

  void setGainDb(float db) {
    gain = std::pow(10.0f, db / 20.0f);
    updateCoeffs();
  }

  float processSample(float x) {
    // Biquad direct form I
    const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x;
    y2 = y1; y1 = y;
    return y;
  }

  void reset() { x1 = x2 = y1 = y2 = 0.0f; }

private:
  void updateCoeffs() {
    const float w0 = 2.0f * static_cast<float>(M_PI) * freqHz / sampleRate;
    const float cosw0 = std::cos(w0);
    const float sinw0 = std::sin(w0);
    const float A = std::sqrt(gain);
    const float alpha = sinw0 / 2.0f * std::sqrt(2.0f);

    const float b0u =    A*((A+1) + (A-1)*cosw0 + 2*std::sqrt(A)*alpha);
    const float b1u = -2*A*((A-1) + (A+1)*cosw0);
    const float b2u =    A*((A+1) + (A-1)*cosw0 - 2*std::sqrt(A)*alpha);
    const float a0u =       (A+1) - (A-1)*cosw0 + 2*std::sqrt(A)*alpha;
    const float a1u =  2*((A-1) - (A+1)*cosw0);
    const float a2u =       (A+1) - (A-1)*cosw0 - 2*std::sqrt(A)*alpha;

    b0 = b0u / a0u; b1 = b1u / a0u; b2 = b2u / a0u;
    a1 = a1u / a0u; a2 = a2u / a0u;
  }

  float sampleRate = 44100.0f;
  float freqHz = 8000.0f;
  float gain = 1.0f;

  float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
  float a1 = 0.0f, a2 = 0.0f;
  float x1 = 0.0f, x2 = 0.0f;
  float y1 = 0.0f, y2 = 0.0f;
};
