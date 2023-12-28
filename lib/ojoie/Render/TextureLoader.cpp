//
// Created by Aleudillonam on 8/17/2022.
//

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Utility/Log.h>


#include "Render/Renderer.hpp"
#include "Render/TextureLoader.hpp"
#include "Allocator/MemoryDefines.h"
#include "HAL/File.hpp"

#include <DirectXTex.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>




namespace AN::RC {


static uint64_t pixelFormatSize(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case kPixelFormatR8Unorm:
        case kPixelFormatR8Unorm_sRGB:
            return 1;
        case kPixelFormatRG8Unorm_sRGB:
            return 2;
        case kPixelFormatRGBA8Unorm:
        case kPixelFormatRGBA8Unorm_sRGB:
            return 4;
        default:
            throw Exception("Invalid enum value");
    }
}

}

namespace AN {


LoadTextureResult::~LoadTextureResult() {
    if (data) {
        if (stb_free) {
            stbi_image_free(data);
        } else {
            AN_FREE(data);
        }
        data = nullptr;
    }
}

}

namespace AN::TextureLoader {

static int stb_io_read_callback(void *user,char *data,int size) {
    File *file = (File *)user;
    return file->Read(data, size);
}

static void stb_io_skip_callback(void *user,int n) {
    File *file = (File *)user;
    file->SetPosition(file->GetPosition() + n);
}

static int stb_io_eof_callback(void *user) {
    File *file = (File *)user;
    return file->GetFileLength() == file->GetPosition() + 1;
}

static void stbi_write_func_file_callback(void *context, void *data, int size) {
    File *file = (File *)context;
    file->Write(data, size);
}

LoadTextureResult __loadTexture(int width, int height, int nrChannels, bool sRgb, unsigned char *data) {

    TextureDescriptor textureDescriptor{};
    unsigned char *bufferMemory;
    unsigned char *buffer = data;

    bool stb_free = true;

    if (nrChannels == 1) {
        if (sRgb) {
            textureDescriptor.pixelFormat = kPixelFormatR8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = kPixelFormatR8Unorm;
        }

    } else if (nrChannels == 3) {

        if (sRgb) {
            textureDescriptor.pixelFormat = kPixelFormatRGBA8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = kPixelFormatRGBA8Unorm;
        }

        uint64_t size = (uint64_t)width * (uint64_t)height;

        bufferMemory = (UInt8 *)AN_MALLOC_ALIGNED(size * 4, 4);

        for (uint64_t i = 0; i < size; ++i) {
            bufferMemory[i * 4] = data[i * 3];
            bufferMemory[i * 4 + 1] = data[i * 3 + 1];
            bufferMemory[i * 4 + 2] = data[i * 3 + 2];
            bufferMemory[i * 4 + 3] = 255;
        }

        buffer = bufferMemory;
        stbi_image_free(data);
        stb_free = false;

    } else if (nrChannels == 4) {
        if (sRgb) {
            textureDescriptor.pixelFormat = kPixelFormatRGBA8Unorm_sRGB;
        } else {
            textureDescriptor.pixelFormat = kPixelFormatRGBA8Unorm;
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

    return LoadTextureResult(stb_free, buffer, width, height, textureDescriptor.pixelFormat, textureDescriptor.mipmapLevel);
}

LoadTextureResult LoadTexture(const char *path, bool sRgb) {

    int width, height, nrChannels;

    stbi_io_callbacks io_callbacks;
    io_callbacks.read = stb_io_read_callback;
    io_callbacks.skip = stb_io_skip_callback;
    io_callbacks.eof = stb_io_eof_callback;

    File file;
    if (!file.Open(path, kFilePermissionRead)) {
        AN_LOG(Error, "cannot open texture file %s", path);
        return {};
    }

    unsigned char *data = stbi_load_from_callbacks(&io_callbacks, &file, &width, &height, &nrChannels, 0);

    if (data) {

        return __loadTexture(width, height, nrChannels, sRgb, data);

    } else {
        ANLog("Failed to load texture at %s", path);
    }


    return {};
}

LoadTextureResult LoadTextureFromMemory(const unsigned char *mem, unsigned int len, bool sRgb) {
    int width, height, nrChannels;
    unsigned char *data = stbi_load_from_memory(mem, len, &width, &height, &nrChannels, 0);

    if (data) {

        return __loadTexture(width, height, nrChannels, sRgb, data);

    } else {
        ANLog("Failed to load texture from memory");
    }

    return {};
}


bool EncodeTexture(const char *path,
                   ImageEncodeType type,
                   void *rawData,
                   UInt32 width, UInt32 height,
                   PixelFormat pixelFormat,
                   int quality) {
    File file;
    if (!file.Open(path, kFilePermissionWrite)) return false;

    int channels;
    switch (pixelFormat) {
        case kPixelFormatR8Unorm:
        case kPixelFormatR8Unorm_sRGB:
            channels = 1;
            break;
        case kPixelFormatRG8Unorm_sRGB:
            channels = 2;
            break;
        case kPixelFormatRGBA8Unorm:
        case kPixelFormatRGBA8Unorm_sRGB:
            channels = 4;
            break;
        default:
            AN_LOG(Error, "%s", "Unsupport Pixel Format");
            return false;
    }

    switch (type) {
        case kImageEncodeTypePNG:
            return stbi_write_png_to_func(stbi_write_func_file_callback, &file,
                                          width, height, channels, rawData, width * RC::pixelFormatSize(pixelFormat));
        case kImageEncodeTypeJPG:
            return stbi_write_jpg_to_func(stbi_write_func_file_callback, &file,
                                      width, height, channels, rawData, quality);
        case kImageEncodeTypeBMP:
            return stbi_write_bmp_to_func(stbi_write_func_file_callback, &file,
                                          width, height, channels, rawData);
    }

    return false;
}

}
