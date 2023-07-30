//
// Created by aojoie on 5/23/2023.
//

#pragma once

#include <ojoie/Render/private/D3D11/Common.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Utility/Utility.h>
#include <ojoie/Render/TextureManager.hpp>

namespace AN::D3D11 {

struct SamplerDescriptorHasher {
    size_t operator() (const SamplerDescriptor &desc) const {
        size_t result = 0;
        Math::hash_combine(result, desc.normalizedCoordinates);
        Math::hash_combine(result, desc.addressModeU);
        Math::hash_combine(result, desc.addressModeV);
        Math::hash_combine(result, desc.addressModeW);
        Math::hash_combine(result, desc.borderColor);
        Math::hash_combine(result, desc.filter);
        Math::hash_combine(result, desc.maxAnisotropy);
        Math::hash_combine(result, desc.compareFunction);
        Math::hash_combine(result, desc.lodMinClamp);
        Math::hash_combine(result, desc.lodMaxClamp);
        return result;
    }
};


struct Texture {
    ComPtr<ID3D11Texture2D> _texture;
    ComPtr<ID3D11ShaderResourceView> _srv;
    SamplerDescriptor samplerDescriptor;
};

/// use std::unique_ptr to avoid rehashing cause pointer change
typedef std::unordered_map<TextureID, std::unique_ptr<Texture>> TextureIDMap;

typedef std::unordered_map<UInt64, ComPtr<ID3D11Resource>>  StagingTextureBuffers;

typedef std::unordered_map<SamplerDescriptor,
                           ComPtr<ID3D11SamplerState>,
                           SamplerDescriptorHasher,
                           memcmp_equal<SamplerDescriptor>>
        SamplerMap;

class TextureManager : public AN::TextureManager {

    TextureIDMap textureIdMap;
    SamplerMap   samplerMap;

    StagingTextureBuffers stagingTextureBuffers;

public:

    /// deallocate all the resources this manager allocated not thread safe
    void deallocTextureResources();

    void uploadTexture2DData(const UInt8 *srcData, ID3D11Texture2D *image,
                             UInt32 width, UInt32 height,
                             PixelFormat format,
                             UInt32      subResource);

    void uploadTexture2D(TextureID tid, const UInt8 *srcData,
                         UInt32 width, UInt32 height,
                         PixelFormat format, bool generateMipmap,
                         const SamplerDescriptor &samplerDescriptor);

    void uploadTextureCube(TextureID tid, UInt8 *srcData, UInt32 faceSize, UInt32 size,
                           PixelFormat format,
                           bool generateMipmap,
                           const SamplerDescriptor &samplerDescriptor);

    void registerRenderTarget(TextureID id, const Texture &tex);

    /// \nullable
    Texture *getTexture(TextureID id);

    /// name should be "white"
    AN::Texture *getTexture(const char *name);

    /// \nonull
    ID3D11SamplerState *getSampler(const SamplerDescriptor &desc);

    void deallocTexture(TextureID id);

    virtual void update(UInt32 version) override {}
};

TextureManager &GetTextureManager();

}
