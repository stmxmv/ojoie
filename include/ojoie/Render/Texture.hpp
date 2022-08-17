//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_TEXTURE_H
#define OJOIE_TEXTURE_H

namespace AN {
class Renderer;
}
namespace AN::RC {

enum class PixelFormat {
    R8Unorm_sRGB,
    RG8Unorm_sRGB,
    RGBA8Unorm_sRGB,
    RGBA8Unorm

};

struct TextureDescriptor {
    uint32_t width, height;
    PixelFormat pixelFormat;
    uint32_t mipmapLevel;
};

class Texture : private NonCopyable {

    struct Impl;
    Impl *impl;

    friend class RenderPipeline;
    friend class AN::Renderer;

    void *getUnderlyingTexture();

public:

    Texture();

    Texture(Texture &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    ~Texture();

    Texture &operator = (Texture &&other) noexcept;

    bool initStatic(const TextureDescriptor &descriptor, void *data, bool generateMipmaps);

    bool initDynamic(const TextureDescriptor &descriptor);

    /// \brief only available when initStatic
    void replaceRegion(uint32_t mipmapLevel, int32_t xOffset, int32_t yOffset, uint32_t width, uint32_t height, void *data);

    void toStatic();

    void deinit();


};



}

#endif//OJOIE_TEXTURE_H
