#pragma once
#include <Audio.h>

class AudioDustPress : public AudioStream {
public:
  AudioDustPress() : AudioStream(2, inputQueueArray) {}
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
  // TODO: env follower, curve bank, pre/post EQ, limiter, smoothers
};