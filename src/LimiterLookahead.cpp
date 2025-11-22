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
  const float incomingAbs = std::fabs(x);
  const float delayed = delay[delayIndex];
  delay[delayIndex] = x;
  delayIndex = (delayIndex + 1) % delay.size();

  const float lookaheadPeak = std::max(incomingAbs, std::fabs(delayed));
  float gain = 1.0f;
  if(lookaheadPeak > ceilingLinear) {
    gain = ceilingLinear / (lookaheadPeak + 1e-12f);
  }

  const float limited = delayed * gain;
  if(std::fabs(limited) <= ceilingLinear) {
    return limited;
  }

  const float clipped = limited > 0 ? ceilingLinear : -ceilingLinear;
  const float overflow = limited - clipped;
  return sat.process(clipped + overflow * 0.25f);
}

void LimiterLookahead::processBlock(float* buffer, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void LimiterLookahead::reset() {
  std::fill(delay.begin(), delay.end(), 0.0f);
  delayIndex = 0;
}

void LimiterLookahead::refreshGain() {
  ceilingLinear = std::pow(10.0f, ceilingDb / 20.0f);
}
