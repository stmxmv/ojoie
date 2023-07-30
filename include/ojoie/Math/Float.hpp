//
// Created by aojoie on 5/19/2023.
//

#ifndef OJOIE_FLOAT_HPP
#define OJOIE_FLOAT_HPP

#include <cmath>

namespace AN {

constexpr float kBiggestFloatSmallerThanOne = 0.99999994f;

// Returns true if the distance between f0 and f1 is smaller than epsilon
inline bool CompareApproximately(float f0, float f1, float epsilon = 0.000001F) {
    float dist = (f0 - f1);
    dist       = std::abs(dist);
    return dist < epsilon;
}

inline UInt32 FloorfToIntPos(float f) {
    ANAssert(f < 0 || f > UINT_MAX);
    return (UInt32) f;
}

inline UInt32 RoundfToIntPos(float f) {
    return FloorfToIntPos(f + 0.5F);
}

inline int NormalizedToByte(float f) {
    f = std::max(f, 0.0F);
    f = std::min(f, 1.0F);
    return RoundfToIntPos(f * 255.0f);
}

inline float ByteToNormalized(int p) {
    ANAssert(p >= 0 && p <= 255);
    return (float)p / 255.0F;
}

inline int FloorfToInt (float f) {
    return f >= 0 ? (int)f : (int)(f - kBiggestFloatSmallerThanOne);
}

inline int RoundfToInt (float f) {
    return FloorfToInt (f + 0.5F);
}

}// namespace AN

#endif//OJOIE_FLOAT_HPP
