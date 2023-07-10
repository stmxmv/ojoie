//
// Created by aojoie on 5/27/2023.
//

#include "Render/Image.hpp"
#include "Core/Exception.hpp"

namespace AN {

UInt64 CalculatePixelFormatSize(PixelFormat pixelFormat, UInt32 width, UInt32 height) {
    switch (pixelFormat) {
        case kPixelFormatR8Unorm:
        case kPixelFormatR8Unorm_sRGB:
            return width * height;
        case kPixelFormatRG8Unorm_sRGB:
            return 2 * width * height;
        case kPixelFormatRGBA8Unorm:
        case kPixelFormatRGBA8Unorm_sRGB:
            return 4 * width * height;

        case kPixelFormatBC1_RGBA:
        case kPixelFormatBC1_RGBA_sRGB:
        case kPixelFormatBC4_RSnorm:
        case kPixelFormatBC4_RUnorm:
        {
            const uint64_t nbw = std::max<uint64_t>(1u, (uint64_t(width) + 3u) / 4u);
            const uint64_t nbh = std::max<uint64_t>(1u, (uint64_t(height) + 3u) / 4u);
            return nbw * nbh * 8U;
        }


        case kPixelFormatBC2_RGBA:
        case kPixelFormatBC2_RGBA_sRGB:
        case kPixelFormatBC3_RGBA:
        case kPixelFormatBC3_RGBA_sRGB:
        case kPixelFormatBC5_RGUnorm:
        case kPixelFormatBC5_RGSnorm:
        case kPixelFormatBC6H_RGBUfloat:
        case kPixelFormatBC6H_RGBFloat:
        case kPixelFormatBC7_RGBAUnorm:
        case kPixelFormatBC7_RGBAUnorm_sRGB:
        {
            const UInt64 nbw = std::max<UInt64>(1u, (UInt64(width) + 3u) / 4u);
            const UInt64 nbh = std::max<UInt64>(1u, (UInt64(height) + 3u) / 4u);
            return nbw * nbh * 16U;
        }
        default:
            throw AN::Exception("Invalid Enum Value");
    }
}

}