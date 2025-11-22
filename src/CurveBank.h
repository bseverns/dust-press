#pragma once

#include <cmath>
#include <cstdint>

// Small collection of drive curves used by DustPress.
// The bank is intentionally tiny; everything is parameterized so the sound can
// still be pushed into weird territory via bias/dirt/chaos.
class CurveBank {
public:
  void setIndex(uint8_t idx) { index = idx; }
  void setBias(float b) { bias = b; }
  void setDirt(float d) { dirt = d; }
  void setChaos(float c) { chaos = c; }

  float process(float x) {
    x += bias;
    x += dirt * x * x * 0.5f;
    x += chaosSample() * chaos;

    switch(index % 3) {
      case 0: return std::tanh(x);
      case 1: return cubicSoftClip(x);
      default: return hardClip(x);
    }
  }

private:
  static float cubicSoftClip(float x) {
    const float x2 = x * x;
    return x - (x2 * x) * 0.333333f;
  }

  static float hardClip(float x) {
    if(x > 1.0f) return 1.0f;
    if(x < -1.0f) return -1.0f;
    return x;
  }

  float chaosSample() {
    // Tiny LCG for deterministic "chaos" modulation.
    chaosState = chaosState * 1664525u + 1013904223u;
    return (static_cast<int32_t>(chaosState >> 9) / 16777216.0f) - 1.0f;
  }

  uint8_t index = 0;
  float bias = 0.0f;
  float dirt = 0.0f;
  float chaos = 0.0f;
  uint32_t chaosState = 1u;
};
