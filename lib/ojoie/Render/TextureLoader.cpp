//
// Created by Aleudillonam on 8/17/2022.
//

#include "Core/typedef.h"

#ifdef OJOIE_USE_VULKAN



#include "Render/TextureLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace AN::TextureLoader {


RC::Texture loadTexture(const char *path) {

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

    RC::TextureDescriptor textureDescriptor{};

    RC::Texture texture;

    if (data) {
        std::unique_ptr<unsigned char[]> bufferMemory;
        unsigned char *buffer = data;

        if (nrChannels == 1) {
            textureDescriptor.pixelFormat = RC::PixelFormat::R8Unorm_sRGB;
        } else if (nrChannels == 3) {

            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm;

            bufferMemory = std::make_unique<unsigned char[]>(width * height * 4);

            uint64_t size = width * height;
            for (uint64_t i = 0; i < size; ++i) {
                bufferMemory[i * 4] = data[i * 3];
                bufferMemory[i * 4 + 1] = data[i * 3 + 1];
                bufferMemory[i * 4 + 2] = data[i * 3 + 2];
                bufferMemory[i * 4 + 3] = 255;
            }

            buffer = bufferMemory.get();

        } else if (nrChannels == 4) {
            textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm_sRGB;
        }

        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.mipmapLevel = (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;


        texture.initStatic(textureDescriptor, buffer, true);

    } else {
        ANLog("Failed to load texture at %s", path);
    }
    stbi_image_free(data);

    return texture;
}

//RC::Texture loadTextureFromMemory(const unsigned char *mem, unsigned int len) {
//
//}


}

#elif defined(OJOIE_USE_OPENGL)
#include "opengl/TextureLoader.cpp"
#endif