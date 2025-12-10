#pragma once

#include <cstddef>
#include <cstdint>
#include "AirEQ.h"
#include "CurveBank.h"
#include "EnvelopeFollower.h"
#include "LimiterLookahead.h"
#include "ParamSmoother.h"
#include "TiltEQ.h"

// Desktop-friendly wrapper that mirrors the Teensy AudioStream version but
// works on interleaved float buffers. This keeps the signal path identical
// while letting us reuse the DSP blocks in a host test bed.
class NativeDustPress {
public:
  explicit NativeDustPress(float sampleRate = 48000.0f);

  void setSampleRate(float sr);

  void setDriveDb(float dB);
  void setBias(float b);
  void setCurveIndex(uint8_t idx);
  void setChaos(float amt);
  void setEnvToDriveDb(float dB);
  void setGateComp(float amt);
  void setPreTilt(float dBPerOct);
  void setPostAir(float gainDb);
  void setDirt(float amt);
  void setCeiling(float dB);
  void setOutputTrimDb(float dB);
  void setMix(float m);

  struct TelemetrySample {
    float env = 0.0f;
    float gateGain = 0.0f;
    float driveDbApplied = 0.0f;
    float limiterEnv = 0.0f;
    float limiterGain = 1.0f;
  };
  std::size_t getLatencySamples() const;

  // Processes a block of stereo audio. Inputs and outputs are separate
  // buffers of size frameCount, using -1..1 normalized samples.
  void processBlock(const float* inLeft,
                    const float* inRight,
                    float* outLeft,
                    float* outRight,
                    std::size_t frameCount);

  // Same processing as above but emits telemetry for each sample so test
  // harnesses can sanity-check the control signals and limiter behavior.
  void processBlockWithTelemetry(const float* inLeft,
                                 const float* inRight,
                                 float* outLeft,
                                 float* outRight,
                                 std::size_t frameCount,
                                 TelemetrySample* telemetry);

  std::size_t getLimiterLookaheadSamples() const { return limiter.lookaheadSamples(); }

  void reset();

private:
  void processBlockInternal(const float* inLeft,
                             const float* inRight,
                             float* outLeft,
                             float* outRight,
                             std::size_t frameCount,
                             TelemetrySample* telemetry);

  void refreshOutputTrim();

  EnvelopeFollower env;
  CurveBank curves;
  TiltEQ tilt;
  TiltEQ::ChannelState tiltLeft;
  TiltEQ::ChannelState tiltRight;
  AirEQ air;
  AirEQ::ChannelState airLeft;
  AirEQ::ChannelState airRight;
  LimiterLookahead limiter;
  ParamSmoother driveSmoother;

  float sampleRate = 48000.0f;
  float driveDb = 0.0f;
  float bias = 0.0f;
  uint8_t curveIndex = 0;
  float envToDriveDb = 0.0f;
  float gateComp = 0.0f;
  float compMakeup = 1.0f;
  float preTiltDbPerOct = 0.0f;
  float postAirDb = 0.0f;
  float dirt = 0.0f;
  float chaos = 0.0f;
  float ceilingDb = -1.0f;
  float outputTrimDb = 0.0f;
  float outputTrimLin = 1.0f;
  float mix = 1.0f;
  float dryMix = 0.0f;
};

