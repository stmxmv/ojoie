//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_MATH_H
#define OJOIE_MATH_H

#include <ojoie/Core/typedef.h>

#define GLM_FORCE_RADIANS

#if !defined(OJOIE_USE_OPENGL)
#define GLM_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace AN::Math {

using namespace glm;

template <typename T>
inline void hash_combine(size_t &seed, const T &v) {
    glm::detail::hash_combine(seed, std::hash<T>{}(v));
}


}


#endif//OJOIE_MATH_H
