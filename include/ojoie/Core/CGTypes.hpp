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
    Int32 x;
    Int32 y;
};

struct Rect {
    Point origin;
    Size size;
    UInt32 width() const { return size.width; }
    UInt32 height() const { return size.height; }
    Int32 x() const { return origin.x; }
    Int32 y() const { return origin.y; }
};

}

#endif//OJOIE_CGTYPES_HPP
