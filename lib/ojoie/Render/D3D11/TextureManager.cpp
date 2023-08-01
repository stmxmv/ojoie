//
// Created by aojoie on 5/23/2023.
//

#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/TextureManager.hpp"
#include "Core/Exception.hpp"
#include "Render/Image.hpp"

#include "Render/Texture2D.hpp"

#include <format>

namespace AN::D3D11 {


void TextureManager::uploadTexture2DData(const UInt8 *srcData,
                                         ID3D11Texture2D *image,
                                         UInt32 width, UInt32 height,
                                         PixelFormat format, UInt32 subResource) {

    UInt64 stagingKey = (UInt64(width) << 0) | (UInt64(height) << 20) | (UInt64(format) << 40);

    auto it = stagingTextureBuffers.find(stagingKey);

    HRESULT hr;
    UInt64 imageBytes = CalculatePixelFormatSize(format, width, height);

    bool isDXT = false;
    if (IsCompressedBCPixelFormat(format)) {
        isDXT = true;
    }


    if (it == stagingTextureBuffers.end()) {
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = toDXGIFormat(format);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        ComPtr<ID3D11Texture2D> texture;

        D3D_ASSERT(hr, GetD3D11Device()->CreateTexture2D(&desc, nullptr, &texture));

        D3D11SetDebugName(texture.Get(), std::format("Staging-Texture2D-{}x{}-fmt={}",  width, height, (int)format));

        auto res = stagingTextureBuffers.insert({ stagingKey, texture });
        ANAssert(res.second);
        it = res.first;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D_ASSERT(hr, GetD3D11Context()->Map(it->second.Get(), 0, D3D11_MAP_WRITE, 0, &mapped));


    char *mappedData = (char *)mapped.pData;
    char *dataPtr = (char *)srcData;

    const int minSize = isDXT ? 4 : 1;
    int mipWidth = std::max((int)width, minSize);
    int mipHeight = std::max((int)height, minSize);
    size_t mappedSize = mapped.RowPitch * mipHeight;

    // pitch for compressed formats is for full row of blocks
    if (isDXT) {
        mappedSize /= 4;
        mipHeight /= 4;
    }

    if (imageBytes == mappedSize) {

        memcpy(mappedData, dataPtr, imageBytes);

    } else {

        const size_t dataRowPitch = imageBytes / height;
        for (int y = 0; y < height; ++y) {
            memcpy(mappedData, dataPtr, dataRowPitch);
            mappedData += mapped.RowPitch;
            dataPtr += dataRowPitch;
        }
    }

    GetD3D11Context()->Unmap(it->second.Get(), 0);

    GetD3D11Context()->CopySubresourceRegion(image, subResource, 0, 0, 0,
                                                   it->second.Get(), 0, nullptr);

#ifdef AN_DEBUG
    AN_LOG(Info, "TexturesD3D11::Upload2DData done");
#endif//AN_DEBUG
}

void TextureManager::uploadTexture2D(AN::TextureID tid
                                     , const UInt8 *srcData,
                                     UInt32 width, UInt32 height,
                                     AN::PixelFormat format,
                                     bool generateMipmap,
                                     const AN::SamplerDescriptor &samplerDescriptor) {

    ANAssert((width != 0 && height != 0));

    auto it = textureIdMap.find(tid);

    Texture *targetTex;
    std::unique_ptr<Texture> tex;

    if (it == textureIdMap.end()) {

        tex = std::make_unique<Texture>();

        UInt32 mipCount;
        if (generateMipmap) {
            mipCount = (uint32_t) (std::floor(std::log2(std::max(width, height)))) + 1;
        } else {
            mipCount = 1;
        }

        /// create texture if not exist
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = mipCount;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = toDXGIFormat(format);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if (generateMipmap) {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        HRESULT hr;
        D3D_ASSERT(hr, GetD3D11Device()->CreateTexture2D(&desc, nullptr, &tex->_texture));

        D3D11SetDebugName(tex->_texture.Get(), std::format("Texture2D-{}-{}x{}", tid, width, height));

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = mipCount;


        D3D_ASSERT(hr, GetD3D11Device()->CreateShaderResourceView(tex->_texture.Get(), nullptr, &tex->_srv));

        D3D11SetDebugName(tex->_srv.Get(), std::format("Texture2D-SRV-{}-{}x{}", tid, width, height));

        tex->samplerDescriptor = samplerDescriptor;
        targetTex = tex.get();

    } else {
        targetTex = it->second.get();
    }

    uploadTexture2DData(srcData, targetTex->_texture.Get(), width, height, format, 0);

    if (generateMipmap) {
        GetD3D11Context()->GenerateMips(targetTex->_srv.Get());
    }

    if (tex) {
        textureIdMap[tid] = std::move(tex);
    }
}


void TextureManager::uploadTextureCube(TextureID tid, UInt8 *srcData,
                                       UInt32 faceSize, UInt32 size,
                                       PixelFormat format,
                                       bool generateMipmap,
                                       const SamplerDescriptor &samplerDescriptor) {

    ID3D11Device* dev = GetD3D11Device();
    ANAssert((size != 0 && faceSize != 0));

    auto it = textureIdMap.find(tid);

    Texture *targetTex;
    std::unique_ptr<Texture> tex;

    UInt32 mipCount;
    if (generateMipmap) {
        mipCount = (uint32_t) (std::floor(std::log2(size))) + 1;
    } else {
        mipCount = 1;
    }

    if (it == textureIdMap.end()) {

        tex = std::make_unique<Texture>();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = size;
        desc.Height = size;
        desc.MipLevels = mipCount;
        desc.ArraySize = 6;
        desc.Format = toDXGIFormat(format);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        if (generateMipmap) {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        HRESULT hr;
        D3D_ASSERT(hr, dev->CreateTexture2D (&desc, NULL, &tex->_texture));
        D3D11SetDebugName(tex->_texture.Get(), std::format("TextureCube-{}-{}x{}", tid, size, size));

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = mipCount;

        D3D_ASSERT(hr,dev->CreateShaderResourceView(tex->_texture.Get(), &viewDesc, &tex->_srv));

        D3D11SetDebugName(tex->_srv.Get(), std::format("TextureCube-SRV-%d-%d", tid, size));

        tex->samplerDescriptor = samplerDescriptor;
        targetTex = tex.get();

    } else {
        targetTex = it->second.get();
    }


    for (int face = 0; face < 6; ++face) {
        UInt8* data = srcData + face * faceSize;
        uploadTexture2DData(data, targetTex->_texture.Get(),
                            size, size, format, D3D11CalcSubresource(0, face, mipCount));
    }

    if (generateMipmap) {
        GetD3D11Context()->GenerateMips(targetTex->_srv.Get());
    }

    if (tex) {
        textureIdMap[tid] = std::move(tex);
    }
}

void TextureManager::registerRenderTarget(TextureID id, const Texture &tex) {
    textureIdMap[id] = std::make_unique<Texture>(tex);
}

Texture *TextureManager::getTexture(TextureID id) {
    if (auto it = textureIdMap.find(id);
        it != textureIdMap.end()) {
        return it->second.get();
    }
    return nullptr;
}

AN::Texture *TextureManager::getTexture(const char *name) {

    static AN::Texture2D *whiteTex = nullptr;

    if (strcmp(name, "white") == 0) {
        if (whiteTex == nullptr) {
            whiteTex = NewObject<AN::Texture2D>();
            TextureDescriptor desc{};
            desc.mipmapLevel = 1;
            desc.pixelFormat = kPixelFormatRGBA8Unorm_sRGB;
            desc.width = 512;
            desc.height = 512;
            whiteTex->init(desc);
            whiteTex->setName("white");

            size_t bytes = 512 * 512 * 4;
            std::unique_ptr<UInt8[]> pixelData = std::make_unique<UInt8[]>(bytes);
            memset(pixelData.get(), 255, bytes);
            whiteTex->setPixelData(pixelData.get());
            whiteTex->uploadToGPU();
        }
        return whiteTex;
    }

    if (strcmp(name, "bump") == 0) {
        static AN::Texture2D *bumpTex = nullptr;
        if (bumpTex == nullptr) {
            bumpTex = NewObject<AN::Texture2D>();
            TextureDescriptor desc{};
            desc.mipmapLevel = 1;
            desc.pixelFormat = kPixelFormatRGBA8Unorm;
            desc.width = 512;
            desc.height = 512;
            bumpTex->init(desc);
            bumpTex->setName("bump");

            size_t bytes = 512 * 512 * 4;
            std::unique_ptr<UInt32[]> pixelData = std::make_unique<UInt32[]>(bytes);
            UInt32 blue = (255 << 24) | (255 << 16) | (128 << 8) | 128; /// little end
            for (int i = 0; i < 512 * 512; ++i) {
                pixelData[i] = blue;
            }
            bumpTex->setPixelData((UInt8 *)pixelData.get());
            bumpTex->uploadToGPU();
        }

        return bumpTex;
    }

    /// not found return the default white tex to avoid crash
    return whiteTex;
}

ID3D11SamplerState *TextureManager::getSampler(const SamplerDescriptor &samplerDescriptor) {
    if (auto it = samplerMap.find(samplerDescriptor); it != samplerMap.end()) {
        return it->second.Get();
    }

    D3D11_SAMPLER_DESC desc{};
    desc.AddressU = toDXGIFormat(samplerDescriptor.addressModeU);
    desc.AddressV = toDXGIFormat(samplerDescriptor.addressModeV);
    desc.AddressW = toDXGIFormat(samplerDescriptor.addressModeW);
    desc.ComparisonFunc = toDXGIFormat(samplerDescriptor.compareFunction);
    desc.MinLOD = samplerDescriptor.lodMinClamp;
    desc.MaxLOD = samplerDescriptor.lodMaxClamp;
    desc.MaxAnisotropy = samplerDescriptor.maxAnisotropy;
    desc.MipLODBias = 0.f;
    desc.Filter = toDXGIFormat(samplerDescriptor.filter);

    switch (samplerDescriptor.borderColor) {
        case kSamplerBorderColorTransparentBlack:
            desc.BorderColor[0] = 0.f;
            desc.BorderColor[1] = 0.f;
            desc.BorderColor[2] = 0.f;
            desc.BorderColor[3] = 0.f;
            break;
        case kSamplerBorderColorOpaqueBlack:
            desc.BorderColor[0] = 0.f;
            desc.BorderColor[1] = 0.f;
            desc.BorderColor[2] = 0.f;
            desc.BorderColor[3] = 1.f;
            break;
        case kSamplerBorderColorOpaqueWhite:
            desc.BorderColor[0] = 1.f;
            desc.BorderColor[1] = 1.f;
            desc.BorderColor[2] = 1.f;
            desc.BorderColor[3] = 1.f;
            break;
        default:
            throw AN::Exception("Invalid Enum Value");
    }

    ComPtr<ID3D11SamplerState> samplerState;
    HRESULT hr;
    D3D_ASSERT(hr, GetD3D11Device()->CreateSamplerState(&desc, &samplerState));

    D3D11SetDebugName(samplerState.Get(),
                      std::format("SamplerState-{}-{}", (int)samplerDescriptor.filter, (int)samplerDescriptor.addressModeU));

    samplerMap.insert({ samplerDescriptor, samplerState });

    return samplerState.Get();
}

void TextureManager::deallocTexture(TextureID id) {
    textureIdMap.erase(id);
}

void TextureManager::deallocTextureResources() {
    samplerMap.clear();
    textureIdMap.clear();
    stagingTextureBuffers.clear();
}

TextureManager &GetTextureManager() {
    static AN::D3D11::TextureManager manager;
    return manager;
}

}