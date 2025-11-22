#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "EnvelopeFollower.h"

// Lightweight lookahead limiter. A circular buffer delays the audio while a
// separate peak envelope drives a gain computer.
class LimiterLookahead {
public:
  LimiterLookahead(float sampleRate = 44100.0f, float lookaheadMs = 2.0f, float releaseMs = 50.0f, float ceilingDb = -0.1f)
      : detector(sampleRate, 0.1f, releaseMs) {
    setSampleRate(sampleRate);
    setLookaheadMs(lookaheadMs);
    setReleaseMs(releaseMs);
    setCeilingDb(ceilingDb);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    detector.setSampleRate(sr);
    resizeBuffer();
  }

  void setLookaheadMs(float ms) {
    lookaheadMs = ms;
    resizeBuffer();
  }

  void setReleaseMs(float ms) {
    detectorRelease = ms;
    detector.setTimesMs(0.1f, ms);
    updateReleaseCoeff();
  }

  void setCeilingDb(float db) { ceiling = std::pow(10.0f, db / 20.0f); }

  float processSample(float x) {
    // Push into delay buffer
    buffer[writeIndex] = x;
    writeIndex = (writeIndex + 1) % buffer.size();

    // Detector runs on the raw input for true peak-ish response
    const float env = detector.processSample(x);
    const float targetGain = std::min(1.0f, ceiling / (env + 1e-6f));
    // Smooth gain only upward to avoid distortion.
    if (targetGain < gainState) {
      gainState = targetGain;
    } else {
      gainState += releaseCoeff * (1.0f - gainState);
      if (gainState > 1.0f)
        gainState = 1.0f;
    }

    const float delayed = buffer[readIndex];
    readIndex = (readIndex + 1) % buffer.size();
    return delayed * gainState;
  }

  void reset(float value = 0.0f) {
    std::fill(buffer.begin(), buffer.end(), value);
    readIndex = writeIndex = 0;
    gainState = 1.0f;
    detector.reset(value);
  }

private:
  void resizeBuffer() {
    const size_t samples = std::max<size_t>(1, static_cast<size_t>(sampleRate * (lookaheadMs / 1000.0f)));
    buffer.assign(samples, 0.0f);
    readIndex = 0;
    writeIndex = samples / 2; // center the latency to avoid silence at start
    updateReleaseCoeff();
  }

  void updateReleaseCoeff() {
    const float eps = 1e-6f;
    releaseCoeff = std::exp(-1.0f / std::max(sampleRate * (detectorReleaseMs() / 1000.0f), eps));
  }

  float detectorReleaseMs() const { return detectorRelease; }

  float sampleRate = 44100.0f;
  float lookaheadMs = 2.0f;
  float ceiling = 0.99f;
  float gainState = 1.0f;
  float releaseCoeff = 0.0f;

  std::vector<float> buffer{1, 0.0f};
  size_t readIndex = 0;
  size_t writeIndex = 0;

  EnvelopeFollower detector;
  float detectorRelease = 50.0f;
};
