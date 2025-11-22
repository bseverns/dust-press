#pragma once
#include <Audio.h>
#include "AirEQ.h"
#include "CurveBank.h"
#include "EnvelopeFollower.h"
#include "LimiterLookahead.h"
#include "ParamSmoother.h"
#include "TiltEQ.h"

class AudioDustPress : public AudioStream {
public:
  AudioDustPress() : AudioStream(2, inputQueueArray) {
    const float sr = AUDIO_SAMPLE_RATE_EXACT;
    env.setSampleRate(sr);
    driveSmoother.setSampleRate(sr);
    driveSmoother.setTimeMs(5.0f);
    tilt.setSampleRate(sr);
    air.setSampleRate(sr);
    limiter.setSampleRate(sr);
  }
  void setDriveDb(float dB);
  void setBias(float b);
  void setCurveIndex(uint8_t idx);
  void setEnvToDriveDb(float dB);
  void setGateComp(float amt);
  void setPreTilt(float dBPerOct);
  void setPostAir(float gainDb);
  void setDirt(float amt);
  void setCeiling(float dB);
  void setOutputTrimDb(float dB);
  void setMix(float m);
  virtual void update();
private:
  audio_block_t *inputQueueArray[2];
  EnvelopeFollower env;
  CurveBank curves;
  TiltEQ tilt;
  AirEQ air;
  LimiterLookahead limiter;
  ParamSmoother driveSmoother;

  float driveDb = 0.0f;
  float bias = 0.0f;
  uint8_t curveIndex = 0;
  float envToDriveDb = 0.0f;
  float gateComp = 0.0f;
  float preTiltDbPerOct = 0.0f;
  float postAirDb = 0.0f;
  float dirt = 0.0f;
  float chaos = 0.0f;
  float ceilingDb = -1.0f;
  float outputTrimDb = 0.0f;
  float mix = 1.0f;
};