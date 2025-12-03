#include "NativeDustPress.h"

#include <algorithm>
#include <cmath>

NativeDustPress::NativeDustPress(float sr) : sampleRate(sr) {
  setSampleRate(sr);
  driveSmoother.setTimeMs(5.0f);
  refreshOutputTrim();
}

void NativeDustPress::setSampleRate(float sr){
  sampleRate = sr;
  env.setSampleRate(sr);
  driveSmoother.setSampleRate(sr);
  tilt.setSampleRate(sr);
  air.setSampleRate(sr);
  limiter.setSampleRate(sr);
}

void NativeDustPress::setDriveDb(float dB){
  if(dB == driveDb) return;
  driveDb = dB;
  driveSmoother.setTarget(driveDb);
}
void NativeDustPress::setBias(float b){
  if(b == bias) return;
  bias = b;
  curves.setBias(bias);
}
void NativeDustPress::setCurveIndex(uint8_t idx){
  if(idx == curveIndex) return;
  curveIndex = idx;
  curves.setIndex(curveIndex);
}
void NativeDustPress::setChaos(float amt){
  if(amt == chaos) return;
  chaos = amt;
  curves.setChaos(chaos);
}
void NativeDustPress::setEnvToDriveDb(float dB){
  if(dB == envToDriveDb) return;
  envToDriveDb = dB;
}
void NativeDustPress::setGateComp(float amt){
  if(amt == gateComp) return;
  gateComp = amt;
  compMakeup = 1.0f + gateComp * 0.2f;
}
void NativeDustPress::setPreTilt(float dBPerOct){
  if(dBPerOct == preTiltDbPerOct) return;
  preTiltDbPerOct = dBPerOct;
  tilt.setSlope(preTiltDbPerOct);
}
void NativeDustPress::setPostAir(float gainDb){
  if(gainDb == postAirDb) return;
  postAirDb = gainDb;
  air.setGainDb(postAirDb);
}
void NativeDustPress::setDirt(float amt){
  if(amt == dirt) return;
  dirt = amt;
  curves.setDirt(dirt);
}
void NativeDustPress::setCeiling(float dB){
  if(dB == ceilingDb) return;
  ceilingDb = dB;
  limiter.setCeilingDb(ceilingDb);
}
void NativeDustPress::setOutputTrimDb(float dB){
  if(dB == outputTrimDb) return;
  outputTrimDb = dB;
  refreshOutputTrim();
}
void NativeDustPress::setMix(float m){
  if(m == mix) return;
  mix = m;
  dryMix = 1.0f - mix;
}

std::size_t NativeDustPress::getLatencySamples() const {
  return limiter.getLatencySamples();
}

void NativeDustPress::processBlock(const float* inLeft,
                                   const float* inRight,
                                   float* outLeft,
                                   float* outRight,
                                   std::size_t frameCount){
  const float gateCompAmt = gateComp;
  const float envDriveAmt = envToDriveDb;
  const float compMakeupLocal = compMakeup;
  const float dryMixLocal = dryMix;
  const float trimLin = outputTrimLin;

  for(std::size_t i=0;i<frameCount;i++){
    const float dryL = inLeft[i];
    const float dryR = inRight[i];
    const float envVal = env.process(0.5f * (std::abs(dryL) + std::abs(dryR)));

    const float gateOpen = envVal * envVal;
    const float gateGain = (1.0f - gateCompAmt) + gateCompAmt * gateOpen;

    float wetL = dryL * gateGain * compMakeupLocal;
    float wetR = dryR * gateGain * compMakeupLocal;

    const float modulatedDrive = driveSmoother.process() + envVal * envDriveAmt;
    const float driveLin = std::pow(10.0f, modulatedDrive / 20.0f);
    wetL *= driveLin;
    wetR *= driveLin;

    wetL = tilt.process(wetL);
    wetR = tilt.process(wetR);

    wetL = curves.process(wetL);
    wetR = curves.process(wetR);

    wetL = air.process(wetL);
    wetR = air.process(wetR);

    wetL = limiter.process(wetL);
    wetR = limiter.process(wetR);

    const float mixedL = (wetL * mix + dryL * dryMixLocal) * trimLin;
    const float mixedR = (wetR * mix + dryR * dryMixLocal) * trimLin;

    outLeft[i] = std::clamp(mixedL, -1.0f, 1.0f);
    outRight[i] = std::clamp(mixedR, -1.0f, 1.0f);
  }
}

void NativeDustPress::reset(){
  env.reset();
  curves.setIndex(curveIndex);
  driveSmoother.reset(driveDb);
  tilt.reset();
  air.reset();
  limiter.reset();
}

void NativeDustPress::refreshOutputTrim(){
  outputTrimLin = std::pow(10.0f, outputTrimDb / 20.0f);
}

