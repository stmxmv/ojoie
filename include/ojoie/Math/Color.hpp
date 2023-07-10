//
// Created by aojoie on 5/25/2023.
//

#pragma once

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Math/Float.hpp>

namespace AN {

class ColorRGBAf {
public:
    float r, g, b, a;

};

class ColorRGBA32 {
public:
    UInt8 r, g, b, a;

    void set(const ColorRGBAf& c) {
        r = NormalizedToByte(c.r);
        g = NormalizedToByte(c.g);
        b = NormalizedToByte(c.b);
        a = NormalizedToByte(c.a);
    }

};

}

