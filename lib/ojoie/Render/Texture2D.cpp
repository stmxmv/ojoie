//
// Created by aojoie on 4/20/2023.
//
#include "Render/Texture2D.hpp"
#include "Render/Image.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/TextureManager.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/TextureManager.hpp"
#include "Allocator/MemoryDefines.h"

namespace AN {

IMPLEMENT_AN_CLASS(Texture2D);
LOAD_AN_CLASS(Texture2D);


Texture2D::~Texture2D() {}

Texture2D::Texture2D(ObjectCreationMode mode) : Super(mode) {
    _texData.data = nullptr;
    bUploadToGPU = false;
}

bool Texture2D::init() {
    throw AN::Exception("Texture2D receive wrong init message");
}

bool Texture2D::init(const TextureDescriptor &desc,
                     const SamplerDescriptor &samplerDescriptor) {
    if (!Super::init()) return false;
    _samplerDescriptor = samplerDescriptor;

    _texData.size = CalculatePixelFormatSize(desc.pixelFormat, desc.width, desc.height);
    _texData.width = desc.width;
    _texData.height = desc.height;
    _texData.pixelFormat = desc.pixelFormat;
    _texData.mipmapLevel = desc.mipmapLevel;

    bIsReadable = false;
    bUploadToGPU = false;
    bSizeChanged = false;

    _texData.data = (UInt8 *)AN_MALLOC_ALIGNED(_texData.size, 4);

    return true;
}

void Texture2D::dealloc() {
    if (bUploadToGPU) {
        if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
            VK::GetTextureManager().deallocTexture(getTextureID());
#endif//OJOIE_USE_VULKAN
        } else {
            D3D11::GetTextureManager().deallocTexture(getTextureID());
        }

        bUploadToGPU = false;
    }

    ANSafeFree(_texData.data);

    Super::dealloc();
}

void Texture2D::uploadToGPU(bool generateMipmap) {
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
            D3D11::GetTextureManager().uploadTexture2D(getTextureID(),
                                                    _texData.data,
                                                    _texData.width, _texData.height,
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

void Texture2D::resize(UInt32 width, UInt32 height) {
    if (_texData.width == width && _texData.height == height) return;
    _texData.width = width;
    _texData.height = height;
    _texData.size = CalculatePixelFormatSize(_texData.pixelFormat, width, height);
    bSizeChanged = true;
    ANSafeFree(_texData.data);
    _texData.data = (UInt8 *)AN_MALLOC_ALIGNED(_texData.size, 4);
}

UInt32 Texture2D::getDataSize() const {
    return CalculatePixelFormatSize(_texData.pixelFormat, _texData.width, _texData.height);
}

void Texture2D::setPixelData(const UInt8 *data) {
    ANAssert(_texData.data != nullptr);
    memcpy(_texData.data, data, _texData.size);
}

template<typename _Coder>
void Texture2D::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(_samplerDescriptor);
    coder.transfer(_texData.width, "width");
    coder.transfer(_texData.height, "height");
    coder.transfer(_texData.mipmapLevel, "mipmapLevel");
    coder.transfer(_texData.pixelFormat, "pixelFormat");

    coder.transferTypeless(_texData.size, "dataSize");
    if constexpr (_Coder::IsDecoding()) {
        ANSafeFree(_texData.data);
        _texData.data = (UInt8 *)AN_MALLOC_ALIGNED(_texData.size, 4);
    }
    coder.transferTypelessData(_texData.data, _texData.size);
}

bool Texture2D::initAfterDecode() {
    if (!Super::initAfterDecode()) {
        return false;
    }

    uploadToGPU();

    return true;
}

IMPLEMENT_AN_OBJECT_SERIALIZE(Texture2D)

}