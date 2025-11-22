#pragma once

#include <cstddef>
#include <cstdint>

// Small collection of drive curves used by DustPress.
// The bank is intentionally tiny; everything is parameterized so the sound can
// still be pushed into weird territory via bias/dirt/chaos.
class CurveBank {
public:
  CurveBank() = default;

  void setIndex(uint8_t idx);
  void setBias(float b);
  void setDirt(float d);
  void setChaos(float c);

  float process(float x);
  void processBlock(float* samples, std::size_t count);

private:
  static float cubicSoftClip(float x);
  static float diodeClip(float x);
  static float foldback(float x);
  static float hardClip(float x);
  float chaosSample();
  float applyCrackle(float x);

  uint8_t index = 0;
  float bias = 0.0f;
  float dirt = 0.0f;
  float chaos = 0.0f;
  uint32_t chaosState = 1u;
};
