//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_MATH_H
#define OJOIE_MATH_H

#define GLM_FORCE_RADIANS
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
