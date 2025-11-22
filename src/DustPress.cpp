#include "DustPress.h"
#include <algorithm>
#include <cmath>

namespace {
inline float dbToLin(float db) {
  return std::pow(10.0f, db / 20.0f);
}

inline float clamp01(float v) {
  return (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
}
}

AudioDustPress::AudioDustPress()
    : AudioStream(2, inputQueueArray),
      curves(),
      envelope(AUDIO_SAMPLE_RATE_EXACT, 5.0f, 50.0f),
      driveSmoother(AUDIO_SAMPLE_RATE_EXACT, 5.0f),
      mixSmoother(AUDIO_SAMPLE_RATE_EXACT, 5.0f),
      preTiltL(AUDIO_SAMPLE_RATE_EXACT, 1200.0f, 0.0f),
      preTiltR(AUDIO_SAMPLE_RATE_EXACT, 1200.0f, 0.0f),
      airL(AUDIO_SAMPLE_RATE_EXACT, 8000.0f, 0.0f),
      airR(AUDIO_SAMPLE_RATE_EXACT, 8000.0f, 0.0f),
      dirtStage(0.0f, 0.0f, 1.0f),
      limiterL(AUDIO_SAMPLE_RATE_EXACT),
      limiterR(AUDIO_SAMPLE_RATE_EXACT) {
  driveSmoother.reset(dbToLin(driveDb));
  mixSmoother.reset(mix);
  dirtStage.setMix(dirtMix);
  limiterL.setCeiling(ceilingDb);
  limiterR.setCeiling(ceilingDb);
}

void AudioDustPress::setDriveDb(float dB) {
  driveDb = dB;
  dirtStage.setDriveDb(driveDb);
}

void AudioDustPress::setBias(float b) {
  bias = b;
  dirtStage.setBias(bias);
}

void AudioDustPress::setCurveIndex(uint8_t idx) {
  curveIndex = idx % CurveBank::kCount;
}

void AudioDustPress::setEnvToDriveDb(float dB) { envToDriveDb = dB; }

void AudioDustPress::setGateComp(float amt) { gateComp = clamp01(amt); }

void AudioDustPress::setPreTilt(float dBPerOct) {
  preTiltDbPerOct = dBPerOct;
  preTiltL.setSlopeDbPerOct(preTiltDbPerOct);
  preTiltR.setSlopeDbPerOct(preTiltDbPerOct);
}

void AudioDustPress::setPostAir(float gainDb) {
  postAirDb = gainDb;
  airL.setGainDb(postAirDb);
  airR.setGainDb(postAirDb);
}

void AudioDustPress::setDirt(float amt) {
  dirtMix = clamp01(amt);
  dirtStage.setMix(dirtMix);
}

void AudioDustPress::setCeiling(float dB) {
  ceilingDb = dB;
  limiterL.setCeiling(ceilingDb);
  limiterR.setCeiling(ceilingDb);
}

void AudioDustPress::setMix(float m) {
  mix = clamp01(m);
  mixSmoother.setTarget(mix);
}

void AudioDustPress::update() {
  audio_block_t *inL = receiveReadOnly(0);
  audio_block_t *inR = receiveReadOnly(1);
  if (!inL || !inR) {
    if (inL)
      release(inL);
    if (inR)
      release(inR);
    return;
  }

  audio_block_t *outL = allocate();
  audio_block_t *outR = allocate();
  if (!outL || !outR) {
    if (outL)
      release(outL);
    if (outR)
      release(outR);
    release(inL);
    release(inR);
    return;
  }

  for (uint32_t i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
    const float inLf = static_cast<float>(inL->data[i]) * kFloatScale;
    const float inRf = static_cast<float>(inR->data[i]) * kFloatScale;
    const float dryMid = 0.5f * (inLf + inRf);

    // Envelope-driven drive morph
    const float env = envelope.processSample(dryMid);
    const float driveTarget = dbToLin(driveDb + env * envToDriveDb);
    driveSmoother.setTarget(driveTarget);
    const float driveLin = driveSmoother.processSample();

    // Downward gate to keep tails tidy
    const float gateScale = 1.0f - gateComp * (1.0f - std::min(1.0f, env / kGateThreshold));

    // Pre-tilt EQ and waveshaper
    const float preL = preTiltL.processSample(inLf);
    const float preR = preTiltR.processSample(inRf);
    const auto curveType = static_cast<CurveBank::CurveType>(curveIndex);
    const float shapedL = curves.processSample(preL, curveType, driveLin, bias);
    const float shapedR = curves.processSample(preR, curveType, driveLin, bias);

    // Optional extra dirt with blend control
    const float dirtL = dirtStage.processSample(shapedL);
    const float dirtR = dirtStage.processSample(shapedR);

    // Air shelf + limiter with stereo link by shared mix / drive
    const float airyL = gateScale * airL.processSample(dirtL);
    const float airyR = gateScale * airR.processSample(dirtR);
    const float limitedL = limiterL.processSample(airyL);
    const float limitedR = limiterR.processSample(airyR);

    const float mixNow = mixSmoother.processSample();
    const float outLf = limitedL * mixNow + inLf * (1.0f - mixNow);
    const float outRf = limitedR * mixNow + inRf * (1.0f - mixNow);

    const float clampedL = std::max(-1.0f, std::min(1.0f, outLf));
    const float clampedR = std::max(-1.0f, std::min(1.0f, outRf));
    outL->data[i] = static_cast<int16_t>(clampedL * 32767.0f);
    outR->data[i] = static_cast<int16_t>(clampedR * 32767.0f);
  }

  transmit(outL, 0);
  transmit(outR, 1);
  release(inL);
  release(inR);
  release(outL);
  release(outR);
}
