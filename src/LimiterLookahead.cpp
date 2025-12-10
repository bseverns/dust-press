#include "LimiterLookahead.h"

#include <algorithm>
#include <cmath>

LimiterLookahead::LimiterLookahead() {
  setChannelCount(1);
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
  lookaheadSamplesCount = std::max<std::size_t>(1, static_cast<std::size_t>((lookaheadMs * 0.001f) * sampleRate));
  resizeDelays();
}

void LimiterLookahead::setChannelCount(std::size_t channels) {
  const std::size_t clamped = std::max<std::size_t>(1, channels);
  channelStates.resize(clamped);
  resizeDelays();
}

float LimiterLookahead::process(float x, std::size_t channel) {
  ChannelState& state = getChannel(channel);
  // Envelope detection on the incoming sample (future relative to delayed path).
  const float incomingAbs = std::fabs(x);
  const float coeff = incomingAbs > state.envelope ? attackCoeff : releaseCoeff;
  state.envelope = coeff * state.envelope + (1.0f - coeff) * incomingAbs;

  const float desiredGain = state.envelope > ceilingLinear ? (ceilingLinear / (state.envelope + 1e-9f)) : 1.0f;
  // Smooth gain toward the smaller of current and desired to avoid overshoot.
  const float target = std::min(desiredGain, state.gainSmooth);
  state.gainSmooth = gainCoeff * state.gainSmooth + (1.0f - gainCoeff) * target;

  // Grab the delayed sample and advance the buffer.
  const float delayed = state.delay[state.delayIndex];
  state.delay[state.delayIndex] = x;
  state.delayIndex = (state.delayIndex + 1) % state.delay.size();

  const float limited = delayed * state.gainSmooth;
  // Final safety soft clip to catch any residual overs.
  return sat.process(limited);
}

void LimiterLookahead::processBlock(float* buffer, std::size_t count, std::size_t channel) {
  for(std::size_t i = 0; i < count; ++i) {
    buffer[i] = process(buffer[i], channel);
  }
}

void LimiterLookahead::reset() {
  for(auto& state : channelStates) {
    std::fill(state.delay.begin(), state.delay.end(), 0.0f);
    state.delayIndex = 0;
    state.envelope = 0.0f;
    state.gainSmooth = 1.0f;
  }
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

void LimiterLookahead::resizeDelays() {
  if(channelStates.empty()) {
    channelStates.resize(1);
  }
  for(auto& state : channelStates) {
    state.delay.assign(lookaheadSamplesCount, 0.0f);
    state.delayIndex = 0;
  }
}

LimiterLookahead::ChannelState& LimiterLookahead::getChannel(std::size_t channel) {
  const std::size_t idx = channelStates.empty() ? 0 : (channel % channelStates.size());
  return channelStates[idx];
}

const LimiterLookahead::ChannelState& LimiterLookahead::getChannel(std::size_t channel) const {
  const std::size_t idx = channelStates.empty() ? 0 : (channel % channelStates.size());
  return channelStates[idx];
}

float LimiterLookahead::currentEnvelope(std::size_t channel) const {
  return getChannel(channel).envelope;
}

float LimiterLookahead::currentGain(std::size_t channel) const {
  return getChannel(channel).gainSmooth;
}
