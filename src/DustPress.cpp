#include "DustPress.h"
#include <cmath>

void AudioDustPress::setDriveDb(float dB){
  driveDb = dB;
  driveSmoother.setTarget(dB);
}
void AudioDustPress::setBias(float b){
  bias = b;
  curves.setBias(bias);
}
void AudioDustPress::setCurveIndex(uint8_t idx){
  curveIndex = idx;
  curves.setIndex(curveIndex);
}
void AudioDustPress::setEnvToDriveDb(float dB){ envToDriveDb = dB; }
void AudioDustPress::setGateComp(float amt){ gateComp = amt; chaos = amt; }
void AudioDustPress::setPreTilt(float dBPerOct){ preTiltDbPerOct = dBPerOct; tilt.setSlope(preTiltDbPerOct); }
void AudioDustPress::setPostAir(float gainDb){ postAirDb = gainDb; air.setGainDb(postAirDb); }
void AudioDustPress::setDirt(float amt){ dirt = amt; curves.setDirt(dirt); }
void AudioDustPress::setCeiling(float dB){ ceilingDb = dB; limiter.setCeilingDb(ceilingDb); }
void AudioDustPress::setMix(float m){ mix = m; }

void AudioDustPress::update(){
  audio_block_t* inL = receiveReadOnly(0);
  audio_block_t* inR = receiveReadOnly(1);
  if(!inL || !inR){ if(inL) release(inL); if(inR) release(inR); return; }
  audio_block_t* outL = allocate();
  audio_block_t* outR = allocate();
  if(!outL || !outR){ if(outL) release(outL); if(outR) release(outR); release(inL); release(inR); return; }

  const float invInt = 1.0f / 32768.0f;
  for(int i=0;i<AUDIO_BLOCK_SAMPLES;i++){
    const float dryL = inL->data[i] * invInt;
    const float dryR = inR->data[i] * invInt;
    const float envVal = env.process(0.5f * (fabsf(dryL) + fabsf(dryR)));

    const float modulatedDrive = driveSmoother.process() + envVal * envToDriveDb;
    const float driveLin = powf(10.0f, modulatedDrive / 20.0f);

    float wetL = dryL * driveLin;
    float wetR = dryR * driveLin;

    wetL = tilt.process(wetL);
    wetR = tilt.process(wetR);

    curves.setChaos(chaos);
    curves.setIndex(curveIndex);
    curves.setBias(bias - gateComp * envVal);
    curves.setDirt(dirt + gateComp * 0.5f);

    wetL = curves.process(wetL);
    wetR = curves.process(wetR);

    wetL = air.process(wetL);
    wetR = air.process(wetR);

    wetL = limiter.process(wetL);
    wetR = limiter.process(wetR);

    const float mixedL = wetL * mix + dryL * (1.0f - mix);
    const float mixedR = wetR * mix + dryR * (1.0f - mix);

    outL->data[i] = static_cast<int16_t>(fmaxf(-1.0f, fminf(1.0f, mixedL)) * 32767.0f);
    outR->data[i] = static_cast<int16_t>(fmaxf(-1.0f, fminf(1.0f, mixedR)) * 32767.0f);
  }

  transmit(outL,0); transmit(outR,1);
  release(inL); release(inR); release(outL); release(outR);
}