//
// Created by aojoie on 5/27/2023.
//

#pragma once

#include <ojoie/Render/RenderTypes.hpp>

namespace AN {

AN_API UInt64 CalculatePixelFormatSize(PixelFormat pixelFormat, UInt32 width, UInt32 height);

inline bool IsCompressedBCPixelFormat(PixelFormat format ) {
    return format >= kPixelFormatBC1_RGBA && format <= kPixelFormatBC7_RGBAUnorm_sRGB;
}

}
