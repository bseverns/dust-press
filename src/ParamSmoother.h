#pragma once

#include <cstddef>

// Quick one-pole smoother for parameter modulation.
class ParamSmoother {
public:
  ParamSmoother() = default;

  void setSampleRate(float sr);
  void setTimeMs(float ms);

  void setTarget(float t);
  float process();
  void processBlock(float* buffer, std::size_t count);

  float getCurrent() const;
  void reset(float value = 0.0f);

private:
  void updateCoeff();

  float sampleRate = 44100.0f;
  float timeMs = 10.0f;
  float coeff = 0.0f;
  float state = 0.0f;
  float target = 0.0f;
};
