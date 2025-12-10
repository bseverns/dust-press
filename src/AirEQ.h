#pragma once

#include <cstddef>

// Tiny high-shelf style "air" control.
class AirEQ {
public:
  struct ChannelState {
    float low = 0.0f;
  };

  AirEQ();

  void setSampleRate(float sr);
  void setGainDb(float g);

  float process(float x);
  float process(float x, ChannelState& state);
  void processBlock(float* buffer, std::size_t count);
  void processBlock(float* buffer, std::size_t count, ChannelState& state);

  void reset(float value = 0.0f);
  void reset(ChannelState& state, float value = 0.0f);

private:
  void updateCoeff();

  float sampleRate = 44100.0f;
  float gainDb = 0.0f;
  float coeff = 0.8f;
  float airGain = 1.0f;

  ChannelState defaultState;
};
