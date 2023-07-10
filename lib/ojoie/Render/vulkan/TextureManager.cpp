//
// Created by aojoie on 4/20/2023.
//

#include "Render/private/vulkan/TextureManager.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/RenderTypes.hpp"

#include <ranges>

namespace AN::VK {

/// max staging buffer with the same size
constexpr static UInt64 kMinStagingTextureBufferSize = 128;

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
            throw AN::Exception("Invalid Enum Value");
    }
}

static bool checkFormatSupport(VkPhysicalDevice gpu,
                               VkFormat format,
                               VkFormatFeatureFlags featureFlags) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(gpu, format, &formatProperties);
    return (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags;
}


TextureManager::~TextureManager() {
    ANAssert(textureIdMap.empty() == true);
    ANAssert(samplerMap.empty());
    ANAssert(threadStagingTextureBuffers.empty());
}

void TextureManager::uploadTexture2DData(const UInt8 *srcData, VkImage image,
                                         UInt32 width, UInt32 height,
                                         PixelFormat format,
                                         UInt32      mipLevel) {

    UInt64 imageBytes = (UInt64) width * height * pixelFormatSize(format);

    StagingTextureBuffers *stagingTextureBuffers;

    {
        std::shared_lock shared_lock(threadStagingTextureBuffers_mutex);

        if (auto it = threadStagingTextureBuffers.find(GetCurrentThreadID());
            it != threadStagingTextureBuffers.end()) {
            stagingTextureBuffers = it->second.get();
        } else {
            shared_lock.unlock();

            std::unique_lock lock(threadStagingTextureBuffers_mutex);
            auto result = threadStagingTextureBuffers.insert({ GetCurrentThreadID(),
                                                                          std::make_unique<StagingTextureBuffers>() });
            stagingTextureBuffers = result.first->second.get();
        }
    }


    auto stagingBuffers = *stagingTextureBuffers |
                          std::views::filter([imageBytes](auto &buffer) { return buffer.buffer.getSize() >= imageBytes; });

    Buffer     *buffer;
    VkSemaphore semaphore;
    UInt64 signalValue;

    /// bufferNum == 0 or less than max num allowed
    if (stagingBuffers.empty()) {

        stagingTextureBuffers->emplace_back();
        auto &staging = stagingTextureBuffers->back();

        BufferDescriptor bufferDescriptor{};
        bufferDescriptor.size           = std::max(imageBytes, kMinStagingTextureBufferSize);
        bufferDescriptor.bufferUsage    = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferDescriptor.allocationFlag = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        bufferDescriptor.memoryUsage    = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        if (!staging.buffer.init(GetDevice(), bufferDescriptor)) {
            return;
        }

        /// init timeline semaphore
        if (!staging.semaphore.init(false)) {
            return;
        }

        buffer    = &staging.buffer;
        semaphore = staging.semaphore.vkSemaphore();
        signalValue = 1;

    } else {

        auto &staging = stagingBuffers.front(); // pointer is valid even if vector move

        /// wait the last copy command complete
        staging.semaphore.wait(staging.lastSemaphoreValue + 1);
        staging.lastSemaphoreValue += 1;

        buffer    = &staging.buffer;
        semaphore = staging.semaphore.vkSemaphore();
        signalValue = staging.lastSemaphoreValue + 1;
    }

    void *dstMem = buffer->map();
    memcpy(dstMem, srcData, imageBytes);
    buffer->flush();
    buffer->unmap();

    CommandBuffer *commandBuffer = GetCommandPool().newCommandBuffer(GetDevice().getGraphicsQueue().getFamilyIndex());

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount     = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel   = mipLevel;
    subresourceRange.levelCount     = 1;

    VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageMemoryBarrier.image            = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    /// command buffer submission results in implicit VK_ACCESS_HOST_WRITE_BIT
    /// and we don't care the src read or write, don't care the cache flushing
    imageMemoryBarrier.srcAccessMask       = 0;
    imageMemoryBarrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT, imageMemoryBarrier);

    commandBuffer->copyBufferToImage(buffer->vkBuffer(),
                                     0, 0, 0, image, mipLevel, 0, 0, width, height, VK_IMAGE_ASPECT_COLOR_BIT);

    /// prepare for fragment shader read
    imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, imageMemoryBarrier);

    commandBuffer->addSignalSemaphore(semaphore, signalValue);
    commandBuffer->submit();
    commandBuffer->waitUntilCompleted();
}

void TextureManager::uploadTexture2D(TextureID tid, const UInt8 *srcData,
                                     UInt32 width, UInt32 height,
                                     PixelFormat format, bool generateMipmap,
                                     const SamplerDescriptor &samplerDescriptor) {

    if (width == 0 || height == 0) {
        return;
    }

    UInt32 mipCount;
    if (generateMipmap) {
        mipCount = (uint32_t) (std::floor(std::log2(std::max(width, height)))) + 1;
    } else {
        mipCount = 1;
    }

    std::shared_lock shared_lock(textureIdMap_mutex);

    auto it = textureIdMap.find(tid);

    Texture *targetTex;
    std::unique_ptr<Texture> tex;

    if (it == textureIdMap.end() || it->second->pendingDestroy) {

        shared_lock.unlock();

        /// texture does not exist yet, create it
        VkImage       image;
        VmaAllocation imageAllocation;
        VkImageView   imageView;

        VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

        image_info.imageType   = VK_IMAGE_TYPE_2D;
        image_info.format      = toVkRenderType(format);
        image_info.extent      = {.width = width, .height = height, .depth = 1U};
        image_info.mipLevels   = mipCount;
        image_info.arrayLayers = 1;
        image_info.samples     = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling      = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage       = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VmaAllocationCreateInfo memory_info{};
        memory_info.usage = VMA_MEMORY_USAGE_AUTO;

        if (!checkFormatSupport(GetDevice().vkPhysicalDevice(),
                                image_info.format,
                                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                VK_FORMAT_FEATURE_BLIT_DST_BIT |
                                VK_FORMAT_FEATURE_TRANSFER_DST_BIT)) {

            AN_LOG(Error, "vulkan image format not support, format enum value %d", image_info.format);
            return;
        }

        VkResult result = vmaCreateImage(GetDevice().vmaAllocator(),
                                         &image_info, &memory_info,
                                         &image, &imageAllocation,
                                         nullptr);
        if (result != VK_SUCCESS) {
            AN_LOG(Error, "Vulkan create image error: %s", ResultCString(result));
            return;
        }

        VkImageViewCreateInfo   view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount     = 1;
        subresourceRange.baseMipLevel   = 0;
        subresourceRange.levelCount     = mipCount;


        view_info.image            = image;
        view_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format           = image_info.format;
        view_info.subresourceRange = subresourceRange;

        result = vkCreateImageView(GetDevice().vkDevice(), &view_info, nullptr, &imageView);

        if (result != VK_SUCCESS) {
            vmaDestroyImage(GetDevice().vmaAllocator(), image, imageAllocation);
            AN_LOG(Error, "Vulkan create image view error: %s", ResultCString(result));
            return;
        }

        tex = std::make_unique<Texture>();
        tex->image                   = image;
        tex->imageAllocation         = imageAllocation;
        tex->imageView               = imageView;

        tex->samplerDescriptor = samplerDescriptor;

        targetTex = tex.get();


    } else {
        targetTex = it->second.get();
        shared_lock.unlock();
    }

    /// srcData may be null to create a empty texture when init
    if (srcData != nullptr) {
        uploadTexture2DData(srcData, targetTex->image, width, height, format, 0);

        if (generateMipmap) {
            CommandBuffer *commandBuffer = GetCommandPool().newCommandBuffer(GetDevice().getGraphicsQueue().getFamilyIndex());
            commandBuffer->generateMipmaps(targetTex->image, width, height, mipCount);
            commandBuffer->submit();
        }
    }

    if (tex) {
        std::unique_lock lock(textureIdMap_mutex); /// change textureID after upload complete
        textureIdMap[tid] = std::move(tex);
    }

}

void TextureManager::deallocTexture(TextureID id) {
    std::unique_lock lock(textureIdMap_mutex);

    auto &tex = *textureIdMap[id];

    GetDevice().waitIdle();
    vkDestroyImageView(GetDevice().vkDevice(), tex.imageView, nullptr);
    vmaDestroyImage(GetDevice().vmaAllocator(), tex.image, tex.imageAllocation);
    textureIdMap.erase(id);
    /// pending destroy
//    _pendingDestroyTextures.emplace_back(id, *textureIdMap[id], _currentVersion);
//    textureIdMap[id]->pendingDestroy = true;
}

Texture *TextureManager::getTexture(TextureID id) {
    std::shared_lock lock(textureIdMap_mutex);
    if (auto it = textureIdMap.find(id);
        it != textureIdMap.end()) {
        return it->second.get();
    }
    return nullptr;
}

VkSampler TextureManager::getSampler(const SamplerDescriptor &samplerDescriptor) {

    {
        std::shared_lock lock(samplerMap_mutex);
        if (auto it = samplerMap.find(samplerDescriptor); it != samplerMap.end()) {
            return it->second;
        }
    }

    VkSampler sampler;

    /// create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.unnormalizedCoordinates = !samplerDescriptor.normalizedCoordinates;
    samplerInfo.addressModeU            = toVkRenderType(samplerDescriptor.addressModeU);
    samplerInfo.addressModeV            = toVkRenderType(samplerDescriptor.addressModeV);
    samplerInfo.addressModeW            = toVkRenderType(samplerDescriptor.addressModeW);
    samplerInfo.borderColor             = toVkRenderType(samplerDescriptor.borderColor);

    /// TODO code break
//    samplerInfo.minFilter               = toVkRenderType(samplerDescriptor.minFilter);
//    samplerInfo.magFilter               = toVkRenderType(samplerDescriptor.magFilter);
//    samplerInfo.mipmapMode              = toVkRenderType(samplerDescriptor.mipFilter);
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = (float) samplerDescriptor.maxAnisotropy;
    samplerInfo.compareEnable           = VK_TRUE;
    samplerInfo.compareOp               = toVkRenderType(samplerDescriptor.compareFunction);
    samplerInfo.minLod                  = samplerDescriptor.lodMinClamp;
    samplerInfo.maxLod                  = samplerDescriptor.lodMaxClamp;
    samplerInfo.mipLodBias              = 0.f;

    if (vkCreateSampler(GetDevice().vkDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        ANLog("failed to create texture sampler!");
        /// return default sampler
        std::shared_lock lock(samplerMap_mutex);
        if (!samplerMap.empty()) {
            return samplerMap.begin()->second;
        }
        return nullptr;
    }

    std::unique_lock lock(samplerMap_mutex);
    samplerMap.insert({samplerDescriptor, sampler});

    return sampler;
}

void TextureManager::registerRenderTarget(TextureID id, const Texture &tex) {
    std::unique_lock lock(textureIdMap_mutex);

    /// note that render target like swapchain target may
    /// register multiple time to change the underlying image
    textureIdMap[id] = std::make_unique<Texture>(tex);
}

void TextureManager::deallocTextureResources() {
    std::scoped_lock lock(textureIdMap_mutex, samplerMap_mutex, threadStagingTextureBuffers_mutex);

    for (auto &tex : textureIdMap | std::views::values) {
        if (tex->imageAllocation) {
            vkDestroyImageView(GetDevice().vkDevice(), tex->imageView, nullptr);
            vmaDestroyImage(GetDevice().vmaAllocator(), tex->image, tex->imageAllocation);
        }
        /// render target image cannot destroy
    }

    /// pending texture's id is in map, no need to destroy
    _pendingDestroyTextures.clear();

    textureIdMap.clear();

    for (VkSampler sampler : samplerMap | std::views::values) {
        vkDestroySampler(GetDevice().vkDevice(), sampler, nullptr);
    }
    samplerMap.clear();
    threadStagingTextureBuffers.clear();
}

void TextureManager::pendingDestroyTextures() {
    for (auto it = _pendingDestroyTextures.begin(); it != _pendingDestroyTextures.end();) {
        auto &[id, tex, ver] = *it;
        if (_currentVersion - ver > 7) {
            GetDevice().waitIdle();
            vkDestroyImageView(GetDevice().vkDevice(), tex.imageView, nullptr);
            vmaDestroyImage(GetDevice().vmaAllocator(), tex.image, tex.imageAllocation);
            it = _pendingDestroyTextures.erase(it);

            {
//                std::unique_lock lock(textureIdMap_mutex);
                /// check if re-upload new image
//                if (textureIdMap[id]->image == tex.image) {
//                    textureIdMap.erase(id);
//                }
            }
        } else {
            ++it;
        }
    }
}

void TextureManager::update(UInt32 version) {
    _currentVersion = version;
    pendingDestroyTextures();
}

TextureManager &GetTextureManager() {
    static TextureManager textureManager;
    return textureManager;
}

}// namespace AN::VK