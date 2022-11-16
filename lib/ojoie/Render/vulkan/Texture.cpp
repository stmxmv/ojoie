//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/Texture.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Image.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {

struct Texture::Impl {
    VK::Image image;
    VK::ImageView imageView;
};

Texture::Texture() : impl(new Impl{}) {}

Texture::~Texture() {
    deinit();
    delete impl;
}

Texture &Texture::operator=(Texture &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete impl;
    impl = other.impl;
    other.impl = nullptr;

    return *this;
}

static VkFormat toVkFormat(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::R8Unorm:
            return VK_FORMAT_R8_UNORM;
        case PixelFormat::R8Unorm_sRGB:
            return VK_FORMAT_R8_SRGB;
        case PixelFormat::RG8Unorm_sRGB:
            return VK_FORMAT_R8G8_SRGB;
        case PixelFormat::RGBA8Unorm_sRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case PixelFormat::RGBA8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
    throw Exception("Invalid Enum Value");
}

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

bool checkFormatSupport(VkPhysicalDevice gpu, VkFormat format, VkFormatFeatureFlags featureFlags) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(gpu, format, &formatProperties);
    return (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags;
}

bool Texture::init(const TextureDescriptor &descriptor) {
    VK::ImageDescriptor imageDescriptor{};
    imageDescriptor.memoryUsage = VMA_MEMORY_USAGE_AUTO;
    imageDescriptor.extent.width = descriptor.width;
    imageDescriptor.extent.height = descriptor.height;
    imageDescriptor.extent.depth = 1;
    imageDescriptor.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    imageDescriptor.format = toVkFormat(descriptor.pixelFormat);
    imageDescriptor.imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageDescriptor.arrayLayers = 1;
    imageDescriptor.mipLevel = descriptor.mipmapLevel;
    imageDescriptor.tiling = VK_IMAGE_TILING_OPTIMAL;

    const RenderContext &context = GetRenderer().getRenderContext();

    if (!checkFormatSupport(context.graphicContext->device->vkPhysicalDevice(), imageDescriptor.format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        ANLog("format not support");
        return false;
    }

    if (!impl->image.init(*context.graphicContext->device, imageDescriptor)) {
        return false;
    }


    if (!impl->imageView.init(impl->image, VK_IMAGE_VIEW_TYPE_2D)) {
        return false;
    }


    return true;
}


//void Texture::replaceRegion(uint32_t mipmapLevel, int32_t xOffset, int32_t yOffset, uint32_t width, uint32_t height, void *data) {
//    void *bufferData;
//    vmaMapMemory(impl->allocator, impl->stagingBufferMemory, &bufferData);
//    memcpy(bufferData, data, width * height * pixelFormatSize(impl->textureDescriptor.pixelFormat));
//    vmaUnmapMemory(impl->allocator, impl->stagingBufferMemory);
//
//    const RenderContext &context = GetRenderer().getRenderContext();
//    copyBufferToImage(context, impl->stagingBuffer, impl->image, mipmapLevel, xOffset, yOffset, width, height);
//}


void Texture::deinit() {
    if (impl) {
        impl->imageView.deinit();
        impl->image.deinit();
    }
}


VK::Image &Texture::getImage() {
    return impl->image;
}

VK::ImageView &Texture::getImageView() {
    return impl->imageView;
}
void *Texture::getUnderlyingTexture() {
    return (void *)impl->imageView.vkImageView();
}

}