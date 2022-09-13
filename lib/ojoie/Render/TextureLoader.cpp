//
// Created by Aleudillonam on 8/17/2022.
//

#include "Core/typedef.h"

#ifdef OJOIE_USE_VULKAN


#include "Render/Renderer.hpp"
#include "Render/Buffer.hpp"
#include "Render/TextureLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


#ifdef _WIN32
#include <Windows.h>
#endif

namespace AN::RC {

static uint64_t pixelFormatSize(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::R8Unorm:
        case PixelFormat::R8Unorm_sRGB:
            return 1;
        case PixelFormat::RG8Unorm_sRGB:
            return 2;
        case PixelFormat::RGBA8Unorm:
        case PixelFormat::RGBA8Unorm_sRGB:
            return 4;
    }
    throw Exception("Invalid Enum Value");
}

}
namespace AN::TextureLoader {


static RC::Texture __loadTexture(int width, int height, int nrChannels, bool sRgb, unsigned char *data) {

    RC::TextureDescriptor textureDescriptor{};

    RC::Texture texture;
    std::unique_ptr<unsigned char[]> bufferMemory;
    unsigned char *buffer = data;

    if (nrChannels == 1) {
        if (sRgb) {
            textureDescriptor.pixelFormat = RC::PixelFormat::R8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = RC::PixelFormat::R8Unorm;
        }

    } else if (nrChannels == 3) {

        if (sRgb) {
            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm;
        }

        uint64_t size = (uint64_t)width * (uint64_t)height;

        bufferMemory = std::make_unique<unsigned char[]>(size * 4);

        for (uint64_t i = 0; i < size; ++i) {
            bufferMemory[i * 4] = data[i * 3];
            bufferMemory[i * 4 + 1] = data[i * 3 + 1];
            bufferMemory[i * 4 + 2] = data[i * 3 + 2];
            bufferMemory[i * 4 + 3] = 255;
        }

        buffer = bufferMemory.get();

    } else if (nrChannels == 4) {
        if (sRgb) {
            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm;
        }
    }

    textureDescriptor.width = width;
    textureDescriptor.height = height;

    bool generateMipmap = nrChannels >= 3;
    if (generateMipmap) {
        textureDescriptor.mipmapLevel = (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;
    } else {
        textureDescriptor.mipmapLevel = 1;
    }

    texture.init(textureDescriptor);


    const RenderContext &context = GetRenderer().getRenderContext();
    uint64_t bufferSize = (uint64_t)textureDescriptor.width * textureDescriptor.height * RC::pixelFormatSize(textureDescriptor.pixelFormat);
    RC::BufferBlock bufferBlock = context.stageBufferPool.bufferBlock(bufferSize);
    RC::BufferAllocation stageBufferAllocation = bufferBlock.allocate(bufferSize);


    memcpy(stageBufferAllocation.map(), buffer, bufferSize);

    stageBufferAllocation.getBuffer().flush();

    RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

    blitCommandEncoder.copyBufferToTexture(stageBufferAllocation.getBuffer(),
                                           stageBufferAllocation.getOffset(), 0, 0, texture, 0, 0, 0, textureDescriptor.width, textureDescriptor.height);

    if (generateMipmap) {
        blitCommandEncoder.generateMipmapsForTexture(texture);
    }


    return texture;
}

RC::Texture loadTexture(const char *path, bool sRgb) {

    int width, height, nrChannels;

#ifdef _WIN32
    std::wstring wFilePath;
    wFilePath.resize(MultiByteToWideChar(CP_UTF8, 0, path, (int)strlen(path) + 1, nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, path, (int)strlen(path) + 1, wFilePath.data(), (int)wFilePath.size());

    FILE *file = _wfopen(wFilePath.c_str(), L"rb");
    unsigned char *data = stbi_load_from_file(file, &width, &height, &nrChannels, 0);
#else
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
#endif

    RC::Texture texture;

    if (data) {

        texture = __loadTexture(width, height, nrChannels, sRgb, data);

    } else {
        ANLog("Failed to load texture at %s", path);
    }

    stbi_image_free(data);

    return texture;
}

RC::Texture loadTextureFromMemory(const unsigned char *mem, unsigned int len, bool sRgb) {
    int width, height, nrChannels;
    unsigned char *data = stbi_load_from_memory(mem, len, &width, &height, &nrChannels, 0);

    RC::Texture texture;

    if (data) {

        texture = __loadTexture(width, height, nrChannels, sRgb, data);

    } else {
        ANLog("Failed to load texture from memory");
    }
    stbi_image_free(data);

    return texture;
}


}

#elif defined(OJOIE_USE_OPENGL)
#include "opengl/TextureLoader.cpp"
#endif