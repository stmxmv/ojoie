//
// Created by aojoie on 5/25/2023.
//

#include "Render/TextureCube.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/TextureManager.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/TextureManager.hpp"
#include "Allocator/MemoryDefines.h"

namespace AN {

IMPLEMENT_AN_CLASS(TextureCube)
LOAD_AN_CLASS(TextureCube)


TextureCube::~TextureCube() {}

TextureCube::TextureCube(ObjectCreationMode mode)
    : Super(mode) {
}

bool TextureCube::init(const TextureDescriptor &desc, const SamplerDescriptor &samplerDescriptor) {
    ANAssert(desc.width == desc.height);
    return Super::init(desc, samplerDescriptor);
}

void TextureCube::setSourceTexture(UInt32 index, Texture2D *tex) {
    ANAssert(index >= 0 && index < 6);
    if (_sourceTextures.empty()) {
        _sourceTextures.resize(6);
    }
    _sourceTextures[index] = tex;
}

void TextureCube::buildFromSources() {
    UInt32 faceSize = getDataSize();
    if (!_texData.data) {
        _texData.data = (UInt8 *)AN_MALLOC_ALIGNED(faceSize * 6, 4);
    }

    for (int i = 0; i < 6; ++i) {
        memcpy(_texData.data + faceSize * i, _sourceTextures[i]->getPixelData(), faceSize);
    }
    _texData.size = faceSize * 6;
}

void TextureCube::uploadToGPU(bool generateMipmap) {
    if (_texData.data) {

        if (bUploadToGPU && bSizeChanged) {
            if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
                VK::GetTextureManager().deallocTexture(getTextureID());
#endif//OJOIE_USE_VULKAN
            } else if (GetGraphicsAPI() == kGraphicsAPID3D11) {
                D3D11::GetTextureManager().deallocTexture(getTextureID());
            }
        }

        if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
            VK::GetTextureManager().uploadTexture2D(getTextureID(),
                                                    _texData.data,
                                                    _texData.width, _texData.height,
                                                    _texData.pixelFormat,
                                                    generateMipmap,
                                                    _samplerDescriptor);
#endif//OJOIE_USE_VULKAN
        } else if (GetGraphicsAPI() == kGraphicsAPID3D11) {
            D3D11::GetTextureManager().uploadTextureCube(getTextureID(),
                                                        _texData.data,
                                                         getDataSize(), _texData.width,
                                                        _texData.pixelFormat,
                                                        generateMipmap,
                                                        _samplerDescriptor);
        }



        bSizeChanged = false;
        bUploadToGPU = true;

        /// destroy cpu data if is not readable
        if (!bIsReadable) {
            ANSafeFree(_texData.data);
        }
    } else {
        AN_LOG(Error, "Not texture data to upload");
    }
}

template<typename _Coder>
void TextureCube::transfer(_Coder &coder) {
    Super::transfer(coder);
//    TRANSFER(_sourceTextures);
}

IMPLEMENT_AN_OBJECT_SERIALIZE(TextureCube)

}