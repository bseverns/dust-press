#pragma once

#include <cstddef>
#include <vector>

#include "SoftSaturation.h"

// Super tiny lookahead limiter — enough for demos/tests.
class LimiterLookahead {
public:
  LimiterLookahead();

  void setSampleRate(float sr);
  void setCeilingDb(float db);
  void setLookaheadMs(float ms);
  void setChannelCount(std::size_t channels);

  float process(float x, std::size_t channel = 0);
  void processBlock(float* buffer, std::size_t count, std::size_t channel = 0);

  float currentEnvelope(std::size_t channel = 0) const;
  float currentGain(std::size_t channel = 0) const;
  std::size_t lookaheadSamples() const { return lookaheadSamplesCount; }
  std::size_t getLatencySamples() const { return lookaheadSamplesCount; }

  void reset();

private:
  struct ChannelState {
    std::vector<float> delay;
    std::size_t delayIndex = 0;
    float envelope = 0.0f;
    float gainSmooth = 1.0f;
  };

  void refreshGain();
  void updateDetectorCoeffs();
  void resizeDelays();
  ChannelState& getChannel(std::size_t channel);
  const ChannelState& getChannel(std::size_t channel) const;

  float sampleRate = 44100.0f;
  float ceilingDb = -1.0f;
  float lookaheadMs = 0.5f;
  float ceilingLinear = 0.89125094f; // -1 dBFS default
  float attackMs = 0.1f;
  float releaseMs = 10.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
  float gainCoeff = 0.0f;
  std::size_t lookaheadSamplesCount = 1;
  std::vector<ChannelState> channelStates;
  SoftSaturation sat;
};
