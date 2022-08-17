//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/Texture.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {

struct Texture::Impl {

    VmaAllocator allocator;
    VkDevice device;
    VkImage image;
    VkImageView imageView;
    VmaAllocation imageAllocation;

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;

    TextureDescriptor textureDescriptor;

    bool createImageView(VkFormat format, uint32_t mipmapLevel) {
        /// create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipmapLevel;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            ANLog("failed to create texture image view!");
            return false;
        }
        return true;
    }
};

Texture::Texture() : impl(new Impl{}) {}

Texture::~Texture() {
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

bool Texture::initStatic(const TextureDescriptor &descriptor, void *data, bool generateMipmaps) {
    const RenderContext &context = GetRenderer().getRenderContext();
    impl->allocator = context.graphicContext->vmaAllocator;
    impl->device = context.graphicContext->logicalDevice;
    impl->textureDescriptor = descriptor;

    uint64_t bytes = descriptor.width * descriptor.height * pixelFormatSize(descriptor.pixelFormat);

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferMemory, nullptr)) {
        ANLog("fail to create staging buffer for vertex buffer");
        return false;
    }

    void *stageData;
    vmaMapMemory(impl->allocator, stagingBufferMemory, &stageData);
    memcpy(stageData, data, bytes);
    vmaUnmapMemory(impl->allocator, stagingBufferMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = descriptor.width;
    imageInfo.extent.height = descriptor.height;
    imageInfo.extent.depth = 1;
    if (generateMipmaps) {
        imageInfo.mipLevels = descriptor.mipmapLevel;
    } else {
        imageInfo.mipLevels = 1;
    }

    imageInfo.arrayLayers = 1;
    imageInfo.format = toVkFormat(descriptor.pixelFormat);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; ///  We will be using a staging buffer instead of a staging image, so this won't be necessary.
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (!checkFormatSupport(context.graphicContext->physicalDevice, imageInfo.format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        ANLog("format not support");
        return false;
    }

    if (vmaCreateImage(impl->allocator, &imageInfo, &allocCreateInfo, &impl->image, &impl->imageAllocation, nullptr) != VK_SUCCESS) {
        ANLog("failed to create VkImage!");
        return false;
    }

    transitionImageLayout(context, impl->image, imageInfo.mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, stagingBuffer, impl->image, 0, 0, 0, descriptor.width, descriptor.height);

    if (generateMipmaps) {
        GenerateMipmaps(context, impl->image, imageInfo.format, descriptor.width, descriptor.height, imageInfo.mipLevels);
    } else {
        transitionImageLayout(context, impl->image, imageInfo.mipLevels, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }



    vmaDestroyBuffer(impl->allocator, stagingBuffer, stagingBufferMemory);


    if (!impl->createImageView(imageInfo.format, imageInfo.mipLevels)) {
        return false;
    }


    return true;
}

bool Texture::initDynamic(const TextureDescriptor &descriptor) {

    const RenderContext &context = GetRenderer().getRenderContext();
    impl->allocator = context.graphicContext->vmaAllocator;
    impl->device = context.graphicContext->logicalDevice;
    impl->textureDescriptor = descriptor;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = descriptor.width * descriptor.height * pixelFormatSize(descriptor.pixelFormat);
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator, &bufferInfo, &allocInfo, &impl->stagingBuffer, &impl->stagingBufferMemory, nullptr)) {
        ANLog("fail to create staging buffer for vertex buffer");
        return false;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = descriptor.width;
    imageInfo.extent.height = descriptor.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = descriptor.mipmapLevel;

    imageInfo.arrayLayers = 1;
    imageInfo.format = toVkFormat(descriptor.pixelFormat);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; ///  We will be using a staging buffer instead of a staging image, so this won't be necessary.
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(impl->allocator, &imageInfo, &allocCreateInfo, &impl->image, &impl->imageAllocation, nullptr) != VK_SUCCESS) {
        ANLog("failed to create VkImage!");
        return false;
    }

    transitionImageLayout(context, impl->image, imageInfo.mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    if (!impl->createImageView(imageInfo.format, imageInfo.mipLevels)) {
        return false;
    }

    return true;
}

void Texture::replaceRegion(uint32_t mipmapLevel, int32_t xOffset, int32_t yOffset, uint32_t width, uint32_t height, void *data) {
    void *bufferData;
    vmaMapMemory(impl->allocator, impl->stagingBufferMemory, &bufferData);
    memcpy(bufferData, data, width * height * pixelFormatSize(impl->textureDescriptor.pixelFormat));
    vmaUnmapMemory(impl->allocator, impl->stagingBufferMemory);

    const RenderContext &context = GetRenderer().getRenderContext();
    copyBufferToImage(context, impl->stagingBuffer, impl->image, mipmapLevel, xOffset, yOffset, width, height);
}

void Texture::toStatic() {
    const RenderContext &context = GetRenderer().getRenderContext();
    transitionImageLayout(context, impl->image, impl->textureDescriptor.mipmapLevel, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vmaDestroyBuffer(impl->allocator, impl->stagingBuffer, impl->stagingBufferMemory);
    impl->stagingBuffer = nullptr;
    impl->stagingBufferMemory = nullptr;
}


void Texture::deinit() {
    if (impl->stagingBuffer) {
        vmaDestroyBuffer(impl->allocator, impl->stagingBuffer, impl->stagingBufferMemory);
    }
    vkDestroyImageView(impl->device, impl->imageView, nullptr);
    vmaDestroyImage(impl->allocator, impl->image, impl->imageAllocation);
}


void *Texture::getUnderlyingTexture() {
    return (void *)impl->imageView;
}

}