//
// Created by aojoie on 4/23/2023.
//

#ifndef OJOIE_CGTYPES_HPP
#define OJOIE_CGTYPES_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {

struct Size {
    UInt32 width;
    UInt32 height;

    bool operator ==(const Size &) const = default;
};

struct Point {
    UInt32 x;
    UInt32 y;
};

struct Rect {
    Point origin;
    Size size;
    UInt32 width() const { return size.width; }
    UInt32 height() const { return size.height; }
    UInt32 x() const { return origin.x; }
    UInt32 y() const { return origin.y; }
};

}

#endif//OJOIE_CGTYPES_HPP
