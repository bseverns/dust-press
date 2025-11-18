#include "DustPress.h"

void AudioDustPress::setDriveDb(float dB){(void)dB;}
void AudioDustPress::setBias(float b){(void)b;}
void AudioDustPress::setCurveIndex(uint8_t idx){(void)idx;}
void AudioDustPress::setEnvToDriveDb(float dB){(void)dB;}
void AudioDustPress::setGateComp(float amt){(void)amt;}
void AudioDustPress::setPreTilt(float dBPerOct){(void)dBPerOct;}
void AudioDustPress::setPostAir(float gainDb){(void)gainDb;}
void AudioDustPress::setDirt(float amt){(void)amt;}
void AudioDustPress::setCeiling(float dB){(void)dB;}
void AudioDustPress::setMix(float m){(void)m;}

void AudioDustPress::update(){
  audio_block_t* inL = receiveReadOnly(0);
  audio_block_t* inR = receiveReadOnly(1);
  if(!inL || !inR){ if(inL) release(inL); if(inR) release(inR); return; }
  audio_block_t* outL = allocate();
  audio_block_t* outR = allocate();
  if(!outL || !outR){ if(outL) release(outL); if(outR) release(outR); release(inL); release(inR); return; }

  // TODO: envelope → drive, pre tilt, curve bank shaper, post air, limiter, mix
  memcpy(outL->data, inL->data, sizeof(outL->data));
  memcpy(outR->data, inR->data, sizeof(outR->data));

  transmit(outL,0); transmit(outR,1);
  release(inL); release(inR); release(outL); release(outR);
}