#pragma once
#include <Audio.h>
#include <array>
#include "AirEQ.h"
#include "CurveBank.h"
#include "EnvelopeFollower.h"
#include "LimiterLookahead.h"
#include "ParamSmoother.h"
#include "SoftSaturation.h"
#include "TiltEQ.h"

class AudioDustPress : public AudioStream {
public:
  AudioDustPress();

  void setDriveDb(float dB);
  void setBias(float b);
  void setCurveIndex(uint8_t idx);
  void setEnvToDriveDb(float dB);
  void setGateComp(float amt);
  void setPreTilt(float dBPerOct);
  void setPostAir(float gainDb);
  void setDirt(float amt);
  void setCeiling(float dB);
  void setMix(float m);

  virtual void update();

private:
  audio_block_t *inputQueueArray[2];

  // Core DSP bricks
  CurveBank curves;
  EnvelopeFollower envelope;
  ParamSmoother driveSmoother;
  ParamSmoother mixSmoother;
  TiltEQ preTiltL;
  TiltEQ preTiltR;
  AirEQ airL;
  AirEQ airR;
  SoftSaturation dirtStage;
  LimiterLookahead limiterL;
  LimiterLookahead limiterR;

  // Parameter state
  float driveDb = 0.0f;
  float bias = 0.0f;
  uint8_t curveIndex = 0;
  float envToDriveDb = 0.0f;
  float gateComp = 0.0f;
  float preTiltDbPerOct = 0.0f;
  float postAirDb = 0.0f;
  float dirtMix = 1.0f;
  float ceilingDb = -0.1f;
  float mix = 1.0f;

  static constexpr float kFloatScale = 1.0f / 32768.0f;
  static constexpr float kGateThreshold = 0.01f;
};
