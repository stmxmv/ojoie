//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_TEXTURELOADER_HPP
#define OJOIE_TEXTURELOADER_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Render/RenderTypes.hpp>

namespace AN {

class AN_API LoadTextureResult {
    bool stb_free;
    UInt8 *data{};
    UInt32 width, height;
    PixelFormat pixelFormat;
    UInt32 mipmapLevel;
public:
    LoadTextureResult() = default;
    LoadTextureResult(bool stbFree,
                      UInt8 *data,
                      UInt32 width, UInt32 height,
                      PixelFormat pixelFormat,
                      UInt32 mipmapLevel)
        : stb_free(stbFree),
          data(data),
          width(width), height(height),
          pixelFormat(pixelFormat),
          mipmapLevel(mipmapLevel) {}

    LoadTextureResult(LoadTextureResult &&other) noexcept
        : stb_free(other.stb_free),
          data(other.data),
          width(other.width),
          height(other.height),
          pixelFormat(other.pixelFormat),
          mipmapLevel(other.mipmapLevel) {
        other.data = nullptr;
    }

    ~LoadTextureResult();

    bool isValid() const { return data != nullptr; }

    UInt8      *getData() const { return data; }
    PixelFormat getPixelFormat() const { return pixelFormat; }
    UInt32      getMipmapLevel() const { return mipmapLevel; }

    UInt32 getWidth() const { return width; }
    UInt32 getHeight() const { return height; }
};

enum ImageEncodeType {
    kImageEncodeTypePNG,
    kImageEncodeTypeJPG,
    kImageEncodeTypeBMP
};


namespace TextureLoader {

/// \RenderActor
AN_API LoadTextureResult LoadTexture(const char *path, bool sRgb = true);

AN_API LoadTextureResult LoadTextureFromMemory(const unsigned char *mem, unsigned int len, bool sRgb = true);

/// \param quality compressed format quality
AN_API bool EncodeTexture(const char     *path,
                          ImageEncodeType type,
                          void           *rawData,
                          UInt32 width, UInt32 height,
                          PixelFormat pixelFormat,
                          int         quality);
};

}

#endif//OJOIE_TEXTURELOADER_HPP
