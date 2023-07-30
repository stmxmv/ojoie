//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_MATH_HPP
#define OJOIE_MATH_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Serialize/SerializeTraits.hpp>
#include <ojoie/Math/Float.hpp>
#ifdef OJOIE_USE_GLM
#include <ojoie/Math/glm/Math.h>
#endif


namespace AN {

inline bool CompareApproximately(Vector4f f0, Vector4f f1, float epsilon = 0.000001F) {
    return CompareApproximately(f0.x, f1.x, epsilon) && CompareApproximately(f0.y, f1.y, epsilon) &&
           CompareApproximately(f0.z, f1.z, epsilon) && CompareApproximately(f0.w, f1.w, epsilon);
}

inline bool CompareApproximately(Vector3f f0, Vector3f f1, float epsilon = 0.000001F) {
    return CompareApproximately(f0.x, f1.x, epsilon) && CompareApproximately(f0.y, f1.y, epsilon) &&
           CompareApproximately(f0.z, f1.z, epsilon);
}

inline bool CompareApproximately(Vector2f f0, Vector2f f1, float epsilon = 0.000001F) {
    return CompareApproximately(f0.x, f1.x, epsilon) && CompareApproximately(f0.y, f1.y, epsilon);
}

}

#endif//OJOIE_MATH_HPP
