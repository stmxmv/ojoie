//
// Created by Aleudillonam on 8/14/2022.
//

#include "ojoie/Configuration/typedef.h"
#include "Render/Texture.hpp"


namespace AN {

IMPLEMENT_AN_CLASS(Texture);
LOAD_AN_CLASS(Texture);

static std::atomic<TextureID> gTextureID;

Texture::Texture(ObjectCreationMode mode) : Super(mode) {}

Texture::~Texture() {}

bool Texture::init() {
    Super::init();
    _textureID = gTextureID++;
    return true;
}

bool Texture::initAfterDecode() {
    if (!Super::initAfterDecode()) return false;
    _textureID = gTextureID++;
    return true;
}

const SamplerDescriptor &Texture::DefaultSamplerDescriptor() {
    static SamplerDescriptor defaultSampler{
        .normalizedCoordinates = true,
        .addressModeU          = kSamplerAddressModeRepeat,
        .addressModeV          = kSamplerAddressModeRepeat,
        .addressModeW          = kSamplerAddressModeRepeat,
        .borderColor           = kSamplerBorderColorOpaqueBlack,
        .filter                = kSamplerFilterTrilinear,
        .maxAnisotropy         = 1,
        .compareFunction       = kCompareFunctionNever,
        .lodMinClamp           = 0.f,
        .lodMaxClamp           = 0.f
    };
    return defaultSampler;
}



}