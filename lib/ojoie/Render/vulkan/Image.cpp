//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/Image.hpp"

#include <cassert>

namespace AN::VK {

Image::Image(AN::VK::Image &&other) noexcept
    : handle(other.handle), memory(other.memory), type(other.type), extent(other.extent),
      format(other.format), usage(other.usage), sample_count(other.sample_count),
      tiling(other.tiling), subresource(other.subresource), _device(other._device), views(std::move(other.views)) {
    other.handle = VK_NULL_HANDLE;
    for (ImageView *view : views) {
        view->_image = this;
    }
}

Image::Image(Device &device,
             VkImage handle,
             const VkExtent3D &extent,
             VkFormat format,
             VkImageUsageFlags image_usage)
    : _device(&device), handle(handle), extent(extent), format(format), usage(image_usage) {

    type = image_type(extent);
    sample_count = VK_SAMPLE_COUNT_1_BIT;

    subresource.mipLevel   = 1;
    subresource.arrayLayer = 1;

    memory = nullptr;
}

bool Image::init(Device &device, const ImageDescriptor &imageDescriptor) {
    _device = &device;

    assert(imageDescriptor.mipLevel > 0 && "Image should have at least one level");
    assert(imageDescriptor.arrayLayers > 0 && "Image should have at least one layer");

    type = image_type(imageDescriptor.extent);
    extent = imageDescriptor.extent;
    format = imageDescriptor.format;
    usage = imageDescriptor.imageUsage;
    sample_count = imageDescriptor.sampleCount;
    tiling = imageDescriptor.tiling;

    subresource.mipLevel   = imageDescriptor.mipLevel;
    subresource.arrayLayer = imageDescriptor.arrayLayers;

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

    image_info.imageType   = type;
    image_info.format      = format;
    image_info.extent      = extent;
    image_info.mipLevels   = imageDescriptor.mipLevel;
    image_info.arrayLayers = imageDescriptor.arrayLayers;
    image_info.samples     = sample_count;
    image_info.tiling      = tiling;
    image_info.usage       = imageDescriptor.imageUsage;
    image_info.initialLayout = imageDescriptor.initialLayout;

    VmaAllocationCreateInfo memory_info{};
    memory_info.usage = imageDescriptor.memoryUsage;
    memory_info.flags = imageDescriptor.allocationFlag;

    if (imageDescriptor.imageUsage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    auto result = vmaCreateImage(_device->vmaAllocator(),
                                 &image_info, &memory_info,
                                 &handle, &memory,
                                 nullptr);

    if (result != VK_SUCCESS) {
        ANLog("Vulkan create image fail");
        return false;
    }

    return true;
}

void Image::deinit() {
    if (handle && memory) {
        vmaDestroyImage(_device->vmaAllocator(), handle, memory);
        handle = nullptr;
        memory = nullptr;
    }
}

Image::~Image() {
    for (ImageView *view : views) {
        view->_image = nullptr;
    }
    deinit();
}

ImageView::ImageView(Device &device, Image &image, VkImageView imageView, VkFormat format)
    : _device(&device), _image(&image), handle(imageView), _format(format) {

    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = image.getSubresource().mipLevel;
    subresourceRange.layerCount = image.getSubresource().arrayLayer;

    if (is_depth_only_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (is_depth_stencil_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    image.views.emplace(this);
}

bool ImageView::init(Image &image, VkImageViewType viewType, VkFormat format) {
    _device = &image.getDevice();
    _image = &image;
    if (format == VK_FORMAT_UNDEFINED) {
        _format = image.getFormat();
    } else {
        _format = format;
    }

    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = image.getSubresource().mipLevel;
    subresourceRange.layerCount = image.getSubresource().arrayLayer;

    if (is_depth_only_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (is_depth_stencil_format(_format)) {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }


    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

    view_info.image            = image.vkImage();
    view_info.viewType         = viewType;
    view_info.format           = _format;
    view_info.subresourceRange = subresourceRange;

    auto result = vkCreateImageView(_device->vkDevice(), &view_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Vulkan cannot create ImageView");
        return false;
    }

    // Register this image view to its image
    // in order to be notified when it gets moved
    image.views.emplace(this);

    return true;
}
void ImageView::deinit() {
    if (handle != VK_NULL_HANDLE) {
        vkDestroyImageView(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
        _image = nullptr;
    }
}
}