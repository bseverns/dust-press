#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// CurveBank collects a handful of characterful transfer curves so the main
// AudioDustPress voice can pick a flavor of dirt without worrying about the
// math each sample. Curves are intentionally simple and light enough for the
// Teensy Audio runtime.
class CurveBank {
public:
  enum CurveType : uint8_t {
    kClean = 0,
    kTanh,
    kSineFold,
    kCubic,
    kHardClip,
    kCount
  };

  CurveBank() = default;

  // Process a single sample using the requested curve. Drive is a linear gain
  // and bias pre-adds DC to change symmetry.
  float processSample(float x, CurveType curve, float drive, float bias) const;

private:
  static inline float softClip(float v) {
    return std::tanh(v);
  }

  static inline float sineFold(float v) {
    // Fold with a short sine for gentle odd harmonics.
    return std::sin(v);
  }

  static inline float cubic(float v) {
    // Odd-order soft clipper.
    return v - (v * v * v) / 3.0f;
  }
};
