#pragma once

#include <cstddef>

// A light-weight envelope follower for control modulation.
// Attack/release are intentionally simple to keep the code size tiny for
// microcontrollers while still giving us a usable contour.
class EnvelopeFollower {
public:
  EnvelopeFollower();

  void setSampleRate(float sr);
  void setAttackMs(float ms);
  void setReleaseMs(float ms);

  float process(float input);
  void processBlock(const float* input, float* envelope, std::size_t count);

  void reset(float value = 0.0f);

  float stateValue() const { return state; }

private:
  void updateCoeffs();

  float sampleRate = 44100.0f;
  float attackMs = 5.0f;
  float releaseMs = 50.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
  float state = 0.0f;
};
