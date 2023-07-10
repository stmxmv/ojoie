//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_MATH_HPP
#define OJOIE_MATH_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Serialize/SerializeTraits.hpp>
#ifdef OJOIE_USE_GLM
#include <ojoie/Math/glm/Math.h>
#endif


namespace AN::Math {

struct rotator {
    float pitch, roll, yaw;
};




}

#endif//OJOIE_MATH_HPP
