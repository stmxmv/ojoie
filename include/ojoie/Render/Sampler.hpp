//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_SAMPLER_HPP
#define OJOIE_SAMPLER_HPP

#include <ojoie/Render/RenderPipelineState.hpp>

namespace AN {
class Renderer;
}

namespace AN::RC {

enum class SamplerAddressMode {
    ClampToEdge,
    MirrorClampToEdge,
    Repeat,
    MirrorRepeat,
    ClampToZero,
    ClampToBorderColor
};

enum class SamplerBorderColor {
    TransparentBlack, /// (0,0,0,0)
    OpaqueBlack, /// (0,0,0,1)
    OpaqueWhite /// (1,1,1,1)
};

enum class SamplerMinMagFilter  {
    Nearest,
    Linear
};

enum class SamplerMipFilter {
    NotMipmapped,
    Nearest,
    Linear
};

struct SamplerDescriptor {

    bool normalizedCoordinates;
    SamplerAddressMode addressModeU;
    SamplerAddressMode addressModeV;
    SamplerAddressMode addressModeW;
    SamplerBorderColor borderColor;
    SamplerMinMagFilter minFilter, magFilter;
    SamplerMipFilter mipFilter;
    uint32_t maxAnisotropy;
    CompareFunction compareFunction;
    float lodMinClamp, lodMaxClamp;

    constexpr inline static SamplerDescriptor Default() {
        return {
                .normalizedCoordinates = true,
                .addressModeU          = SamplerAddressMode::Repeat,
                .addressModeV          = SamplerAddressMode::Repeat,
                .addressModeW          = SamplerAddressMode::Repeat,
                .borderColor           = SamplerBorderColor::OpaqueBlack,
                .minFilter             = SamplerMinMagFilter::Linear,
                .magFilter             = SamplerMinMagFilter::Linear,
                .mipFilter             = SamplerMipFilter::Linear,
                .maxAnisotropy         = 1,
                .compareFunction       = CompareFunction::Never,
                .lodMinClamp           = 0.f,
                .lodMaxClamp           = 0.f
        };
    }
};
class Sampler : private NonCopyable {

    struct Impl;
    Impl *impl;

    friend class RenderPipeline;
    friend class AN::Renderer;

    void *getUnderlyingSampler() const;

#ifdef OJOIE_USE_VULKAN
    friend class VK::RenderCommandEncoder;
#endif

public:

    Sampler();

    Sampler(Sampler &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    ~Sampler();


    bool init(const SamplerDescriptor &samplerDescriptor);

    void deinit();


};

}

#endif//OJOIE_SAMPLER_HPP
