#include "CurveBank.h"

#include <cmath>
#include <algorithm>

void CurveBank::setIndex(uint8_t idx) { index = idx; }
void CurveBank::setBias(float b) { bias = b; }
void CurveBank::setDirt(float d) { dirt = d; }
void CurveBank::setChaos(float c) { chaos = c; }

float CurveBank::process(float x) {
  // Bias shifts the operating point for odd/even flavor.
  float shaped = x + bias;

  // Dirt pre-emphasizes magnitude so the curves hit harder without hard-clipping.
  shaped += dirt * shaped * shaped * 0.5f * (shaped >= 0.0f ? 1.0f : -1.0f);

  // Chaos adds jitter before the curve and occasional crackle after.
  const float chaosNorm = std::clamp(chaos, 0.0f, 7.0f) / 7.0f;
  shaped += chaosSample() * (0.015f * chaosNorm);

  float y = shaped;
  switch(index & 0x3) {
    case 0: y = std::tanh(shaped); break;                // tape-ish
    case 1: y = cubicSoftClip(shaped); break;            // console-ish
    case 2: y = diodeClip(shaped); break;                // diode pair vibe
    case 3: y = foldback(shaped); break;                 // metallic fold
    default: y = hardClip(shaped); break;
  }

  return applyCrackle(y);
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

float CurveBank::diodeClip(float x) {
  // Simple exponential diode transfer: quiet stays clean, peaks bend asymmetrically.
  const float alpha = 3.5f;
  if(x >= 0.0f) {
    return 1.0f - std::exp(-alpha * x);
  }
  return -(1.0f - std::exp(alpha * x));
}

float CurveBank::foldback(float x) {
  const float threshold = 1.0f;
  float magnitude = std::fabs(x);
  if(magnitude <= threshold) {
    return x;
  }
  const float folded = std::fmod(magnitude - threshold, threshold * 2.0f);
  const float signedFold = (folded <= threshold ? folded : (threshold * 2.0f - folded));
  return (x >= 0.0f ? 1.0f : -1.0f) * signedFold;
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

float CurveBank::applyCrackle(float x) {
  if(chaos <= 0.0f) {
    return x;
  }

  // Crackle probability increases with chaos, spike magnitude stays modest.
  const float chaosNorm = std::clamp(chaos, 0.0f, 7.0f) / 7.0f;
  chaosState = chaosState * 1103515245u + 12345u;
  const float rand01 = (chaosState & 0x00FFFFFFu) / 16777216.0f;
  const float probability = 0.01f + chaosNorm * 0.04f;
  if(rand01 < probability) {
    const float spike = ((chaosState >> 8) & 0x00FFFFFFu) / 8388608.0f - 1.0f;
    return hardClip(x + spike * (0.1f + 0.2f * chaosNorm));
  }

  return x;
}
