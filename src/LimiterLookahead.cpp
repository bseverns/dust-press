#include "LimiterLookahead.h"

#include <algorithm>
#include <cmath>

LimiterLookahead::LimiterLookahead() {
  setSampleRate(sampleRate);
  refreshGain();
}

void LimiterLookahead::setSampleRate(float sr) {
  sampleRate = sr;
  setLookaheadMs(lookaheadMs);
  updateDetectorCoeffs();
}

void LimiterLookahead::setCeilingDb(float db) {
  ceilingDb = db;
  refreshGain();
}

void LimiterLookahead::setLookaheadMs(float ms) {
  lookaheadMs = ms;
  const std::size_t delaySamples = std::max<std::size_t>(1, static_cast<std::size_t>((lookaheadMs * 0.001f) * sampleRate));
  delay.assign(delaySamples, 0.0f);
  delayIndex = 0;
}

float LimiterLookahead::process(float x) {
  // Envelope detection on the incoming sample (future relative to delayed path).
  const float incomingAbs = std::fabs(x);
  const float coeff = incomingAbs > envelope ? attackCoeff : releaseCoeff;
  envelope = coeff * envelope + (1.0f - coeff) * incomingAbs;

  const float desiredGain = envelope > ceilingLinear ? (ceilingLinear / (envelope + 1e-9f)) : 1.0f;
  // Smooth gain toward the smaller of current and desired to avoid overshoot.
  const float target = std::min(desiredGain, gainSmooth);
  gainSmooth = gainCoeff * gainSmooth + (1.0f - gainCoeff) * target;

  // Grab the delayed sample and advance the buffer.
  const float delayed = delay[delayIndex];
  delay[delayIndex] = x;
  delayIndex = (delayIndex + 1) % delay.size();

  const float limited = delayed * gainSmooth;
  // Final safety soft clip to catch any residual overs.
  return sat.process(limited);
}

void LimiterLookahead::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void LimiterLookahead::reset() {
  std::fill(delay.begin(), delay.end(), 0.0f);
  delayIndex = 0;
  envelope = 0.0f;
  gainSmooth = 1.0f;
}

void LimiterLookahead::refreshGain() {
  ceilingLinear = std::pow(10.0f, ceilingDb / 20.0f);
}

void LimiterLookahead::updateDetectorCoeffs() {
  const float attackSamples = (attackMs * 0.001f) * sampleRate;
  const float releaseSamples = (releaseMs * 0.001f) * sampleRate;
  attackCoeff = attackSamples > 1.0f ? std::exp(-1.0f / attackSamples) : 0.0f;
  releaseCoeff = releaseSamples > 1.0f ? std::exp(-1.0f / releaseSamples) : 0.0f;

  // Gain smoothing follows the detector release time to keep pumping gentle.
  gainCoeff = releaseCoeff;
}
