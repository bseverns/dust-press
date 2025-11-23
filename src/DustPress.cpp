#include "DustPress.h"
#include <cmath>

void AudioDustPress::setDriveDb(float dB){
  if(dB == driveDb) return;
  driveDb = dB;
  driveSmoother.setTarget(dB);
}
void AudioDustPress::setBias(float b){
  if(b == bias) return;
  bias = b;
  curves.setBias(bias);
}
void AudioDustPress::setCurveIndex(uint8_t idx){
  if(idx == curveIndex) return;
  curveIndex = idx;
  curves.setIndex(curveIndex);
}
void AudioDustPress::setChaos(float amt){
  if(amt == chaos) return;
  chaos = amt;
  curves.setChaos(chaos);
}
void AudioDustPress::setEnvToDriveDb(float dB){
  if(dB == envToDriveDb) return;
  envToDriveDb = dB;
}
void AudioDustPress::setGateComp(float amt){
  if(amt == gateComp) return;
  gateComp = amt;
  compMakeup = 1.0f + gateComp * 0.2f;
}
void AudioDustPress::setPreTilt(float dBPerOct){
  if(dBPerOct == preTiltDbPerOct) return;
  preTiltDbPerOct = dBPerOct;
  tilt.setSlope(preTiltDbPerOct);
}
void AudioDustPress::setPostAir(float gainDb){
  if(gainDb == postAirDb) return;
  postAirDb = gainDb;
  air.setGainDb(postAirDb);
}
void AudioDustPress::setDirt(float amt){
  if(amt == dirt) return;
  dirt = amt;
  curves.setDirt(dirt);
}
void AudioDustPress::setCeiling(float dB){
  if(dB == ceilingDb) return;
  ceilingDb = dB;
  limiter.setCeilingDb(ceilingDb);
}
void AudioDustPress::setOutputTrimDb(float dB){
  if(dB == outputTrimDb) return;
  outputTrimDb = dB;
  outputTrimLin = powf(10.0f, outputTrimDb / 20.0f);
}
void AudioDustPress::setMix(float m){
  if(m == mix) return;
  mix = m;
  dryMix = 1.0f - mix;
}

void AudioDustPress::update(){
  audio_block_t* inL = receiveReadOnly(0);
  audio_block_t* inR = receiveReadOnly(1);
  if(!inL || !inR){ if(inL) release(inL); if(inR) release(inR); return; }
  audio_block_t* outL = allocate();
  audio_block_t* outR = allocate();
  if(!outL || !outR){ if(outL) release(outL); if(outR) release(outR); release(inL); release(inR); return; }

  const float invInt = 1.0f / 32768.0f;
  const float trimLin = outputTrimLin;
  const float gateCompAmt = gateComp;
  const float envDriveAmt = envToDriveDb;
  const float compMakeupLocal = compMakeup;
  const float dryMixLocal = dryMix;

  for(int i=0;i<AUDIO_BLOCK_SAMPLES;i++){
    const float dryL = inL->data[i] * invInt;
    const float dryR = inR->data[i] * invInt;
    const float envVal = env.process(0.5f * (fabsf(dryL) + fabsf(dryR)));

    // Gate/comp opens with the envelope, lifts quiet tails, then feeds the shaper.
    const float gateOpen = envVal * envVal;
    const float gateGain = (1.0f - gateCompAmt) + gateCompAmt * gateOpen;

    float wetL = dryL * gateGain * compMakeupLocal;
    float wetR = dryR * gateGain * compMakeupLocal;

    // Envelope-driven drive modulation.
    const float modulatedDrive = driveSmoother.process() + envVal * envDriveAmt;
    const float driveLin = powf(10.0f, modulatedDrive / 20.0f);
    wetL *= driveLin;
    wetR *= driveLin;

    // Pre-tilt EQ before the shaper.
    wetL = tilt.process(wetL);
    wetR = tilt.process(wetR);

    // Bias + chaos/dirt flavored curves.
    wetL = curves.process(wetL);
    wetR = curves.process(wetR);

    // Post-air EQ and safety limiting.
    wetL = air.process(wetL);
    wetR = air.process(wetR);

    wetL = limiter.process(wetL);
    wetR = limiter.process(wetR);

    const float mixedL = (wetL * mix + dryL * dryMixLocal) * trimLin;
    const float mixedR = (wetR * mix + dryR * dryMixLocal) * trimLin;

    outL->data[i] = static_cast<int16_t>(fmaxf(-1.0f, fminf(1.0f, mixedL)) * 32767.0f);
    outR->data[i] = static_cast<int16_t>(fmaxf(-1.0f, fminf(1.0f, mixedR)) * 32767.0f);
  }

  transmit(outL,0); transmit(outR,1);
  release(inL); release(inR); release(outL); release(outR);
}