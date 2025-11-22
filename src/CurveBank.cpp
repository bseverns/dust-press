#include "CurveBank.h"
#include <algorithm>

float CurveBank::processSample(float x, CurveBank::CurveType curve, float drive, float bias) const {
  const float driven = (x + bias) * drive;
  switch (curve) {
  case kClean:
    return driven;
  case kTanh:
    return softClip(driven);
  case kSineFold:
    return sineFold(driven);
  case kCubic:
    return cubic(driven);
  case kHardClip: {
    const float clipped = std::max(-1.0f, std::min(1.0f, driven));
    return clipped;
  }
  default:
    return driven;
  }
}
