#include "EnvelopeFollower.h"

#include <cmath>

EnvelopeFollower::EnvelopeFollower() { reset(); }

void EnvelopeFollower::setSampleRate(float sr) {
  sampleRate = sr;
  updateCoeffs();
}

void EnvelopeFollower::setAttackMs(float ms) {
  attackMs = ms;
  updateCoeffs();
}

void EnvelopeFollower::setReleaseMs(float ms) {
  releaseMs = ms;
  updateCoeffs();
}

float EnvelopeFollower::process(float input) {
  const float rectified = std::fabs(input);
  const float coeff = rectified > state ? attackCoeff : releaseCoeff;
  state = coeff * state + (1.0f - coeff) * rectified;
  return state;
}

void EnvelopeFollower::processBlock(const float* input, float* envelope, std::size_t count) {
  for(std::size_t i = 0; i < count; ++i) {
    envelope[i] = process(input[i]);
  }
}

void EnvelopeFollower::reset(float value) {
  state = value;
  updateCoeffs();
}

void EnvelopeFollower::updateCoeffs() {
  const float attackSamples = (attackMs * 0.001f) * sampleRate;
  const float releaseSamples = (releaseMs * 0.001f) * sampleRate;
  attackCoeff = attackSamples > 1.0f ? std::exp(-1.0f / attackSamples) : 0.0f;
  releaseCoeff = releaseSamples > 1.0f ? std::exp(-1.0f / releaseSamples) : 0.0f;
}
