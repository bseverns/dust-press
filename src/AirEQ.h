#pragma once

#include <cstddef>

// Tiny high-shelf style "air" control.
class AirEQ {
public:
  AirEQ() = default;

  void setSampleRate(float sr);
  void setGainDb(float g);

  float process(float x);
  void processBlock(float* buffer, std::size_t count);

  void reset(float value = 0.0f);

private:
  void updateCoeff();

  float sampleRate = 44100.0f;
  float gainDb = 0.0f;
  float coeff = 0.8f;
  float low = 0.0f;
};
