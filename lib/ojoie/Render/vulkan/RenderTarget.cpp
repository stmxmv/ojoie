//
// Created by aojoie on 4/20/2023.
//

#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/RenderTypes.hpp"

namespace AN::VK {

RenderTarget::RenderTarget()
    : _extent(),
      _image(),
      _imageAllocation(),
      _imageView(),
      _format(),
      _samples(),
      _imageCreateFlags(),
      _imageUsage(),
      bIsSwapchain() {}

RenderTarget::RenderTarget(RenderTarget &&other) noexcept
    : _extent(other._extent),
      _image(other._image),
      _imageAllocation(other._imageAllocation),
      _imageView(other._imageView),
      _format(other._format),
      _samples(other._samples),
      _imageCreateFlags(other._imageCreateFlags),
      _imageUsage(other._imageUsage),
      bIsSwapchain(other.bIsSwapchain) {
    other._image = nullptr;
    other._imageAllocation = nullptr;
    other._imageView = nullptr;
}

bool RenderTarget::init(VkFormat format, const VkExtent2D &extent,
                        VkSampleCountFlagBits samples) {

    bIsSwapchain = false;
    _format      = format;
    _samples     = samples;
    _extent      = extent;

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

    image_info.imageType   = VK_IMAGE_TYPE_2D;
    image_info.format      = format;
    image_info.extent      = {.width = extent.width, .height = extent.height, .depth = 1U};
    image_info.mipLevels   = 1;
    image_info.arrayLayers = 1;
    image_info.samples     = samples;
    image_info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    image_info.flags = GetImageCreateFlags(format);
    image_info.usage = GetImageUsageFlags(format);

    VmaAllocationCreateInfo memory_info{};
    memory_info.usage          = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
//    memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

    _imageCreateFlags = image_info.flags;
    _imageUsage       = image_info.usage;

    VkResult result = vmaCreateImage(GetDevice().vmaAllocator(),
                                     &image_info, &memory_info,
                                     &_image, &_imageAllocation,
                                     nullptr);
    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Vulkan create image error: %s", ResultCString(result));
        return false;
    }

    VkImageViewCreateInfo   view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkImageSubresourceRange subresourceRange{};

    if (is_depth_only_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (is_depth_stencil_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;


    view_info.image            = _image;
    view_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format           = format;
    view_info.subresourceRange = subresourceRange;

    result = vkCreateImageView(GetDevice().vkDevice(), &view_info, nullptr, &_imageView);

    if (result != VK_SUCCESS) {
        vmaDestroyImage(GetDevice().vmaAllocator(), _image, _imageAllocation);
        AN_LOG(Error, "Vulkan create swapchain image view error: %s", ResultCString(result));
        return false;
    }

    return true;
}

bool RenderTarget::init(const AttachmentDescriptor &attachmentDescriptor) {

    return init(toVkRenderType(attachmentDescriptor.format),
                { .width = attachmentDescriptor.width, .height = attachmentDescriptor.height },
                (VkSampleCountFlagBits)attachmentDescriptor.samples);
}

void RenderTarget::resize(const VkExtent2D &extent) {
    ANAssert(bIsSwapchain == false);
    deinit();
    ANAssert(init(_format, extent, _samples));
}

void RenderTarget::deinit() {
    if (_imageView) {
        vkDestroyImageView(GetDevice().vkDevice(), _imageView, nullptr);
        _imageView = nullptr;
    }

    if (!bIsSwapchain) {
        if (_image) {
            vmaDestroyImage(GetDevice().vmaAllocator(), _image, _imageAllocation);
            _image = nullptr;
        }
    }
}

void RenderTarget::bridgeSwapchainRenderTarget(VkImage            image,
                                               const VkExtent2D  &extent,
                                               VkFormat           format,
                                               VkImageCreateFlags imageCreateflags,
                                               VkImageUsageFlags  imageUsage) {
    deinit();

    _image  = image;
    _format = format;
    _extent = extent;
    /// swapchain image samples must be 1
    _samples = VK_SAMPLE_COUNT_1_BIT;

    _imageCreateFlags = imageCreateflags;
    _imageUsage       = imageUsage;

    bIsSwapchain = true;

    /// create image views
    VkImageViewCreateInfo   view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;


    view_info.image            = image;
    view_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format           = format;
    view_info.subresourceRange = subresourceRange;

    VkResult result = vkCreateImageView(GetDevice().vkDevice(), &view_info, nullptr, &_imageView);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Vulkan create swapchain image view error: %s", ResultCString(result));
        return;
    }
}

VkImageCreateFlags RenderTarget::GetImageCreateFlags(VkFormat format) {
    return 0;
}

VkImageUsageFlags RenderTarget::GetImageUsageFlags(VkFormat format) {
    if (is_depth_stencil_format(format)) {
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
}


}// namespace AN::VK