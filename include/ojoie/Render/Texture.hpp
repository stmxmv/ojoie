//
// Created by Aleudillonam on 8/10/2022.
//

#ifndef OJOIE_TEXTURE_H
#define OJOIE_TEXTURE_H

namespace AN {
class Renderer;
}

#ifdef OJOIE_USE_VULKAN
namespace AN::VK {
class RenderCommandEncoder;
class Image;
class ImageView;
}
#endif

namespace AN::RC {

class BlitCommandEncoder;

enum class PixelFormat {
    R8Unorm,
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

#ifdef OJOIE_USE_VULKAN
    friend class AN::VK::RenderCommandEncoder;
    friend class AN::RC::BlitCommandEncoder;

    VK::Image &getImage();
    VK::ImageView &getImageView();
#endif
public:

    Texture();

    Texture(Texture &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    ~Texture();

    Texture &operator = (Texture &&other) noexcept;

    bool init(const TextureDescriptor &descriptor);

//    / \brief only available when initStatic
//    void replaceRegion(uint32_t mipmapLevel, int32_t xOffset, int32_t yOffset, uint32_t width, uint32_t height, void *data);

    void deinit();


};



}

#endif//OJOIE_TEXTURE_H
