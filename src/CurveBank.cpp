#include "CurveBank.h"

#include <cmath>

void CurveBank::setIndex(uint8_t idx) { index = idx; }
void CurveBank::setBias(float b) { bias = b; }
void CurveBank::setDirt(float d) { dirt = d; }
void CurveBank::setChaos(float c) { chaos = c; }

float CurveBank::process(float x) {
  x += bias;
  x += dirt * x * x * 0.5f;
  x += chaosSample() * chaos;

  switch(index % 3) {
    case 0: return std::tanh(x);
    case 1: return cubicSoftClip(x);
    default: return hardClip(x);
  }
}

void CurveBank::processBlock(float* samples, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    samples[i] = process(samples[i]);
  }
}

float CurveBank::cubicSoftClip(float x) {
  const float x2 = x * x;
  return x - (x2 * x) * 0.333333f;
}

float CurveBank::hardClip(float x) {
  if(x > 1.0f) return 1.0f;
  if(x < -1.0f) return -1.0f;
  return x;
}

float CurveBank::chaosSample() {
  // Tiny LCG for deterministic "chaos" modulation.
  chaosState = chaosState * 1664525u + 1013904223u;
  return (static_cast<int32_t>(chaosState >> 9) / 16777216.0f) - 1.0f;
}
