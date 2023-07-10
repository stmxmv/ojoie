//
// Created by Aleudillonam on 8/26/2022.
//

#ifndef OJOIE_VK_IMAGE_HPP
#define OJOIE_VK_IMAGE_HPP


#include "ojoie/Configuration/typedef.h"

#include "ojoie/Render/private/vulkan.hpp"

#include <stdexcept>
#include <vector>
#include <unordered_set>

namespace AN::VK {

class Device;

struct ImageDescriptor {
    typedef ImageDescriptor Self;
    VkExtent3D extent;
    VkFormat format;
    VkImageUsageFlags imageUsage;
    VmaMemoryUsage memoryUsage;
    VmaAllocationCreateFlags allocationFlag;
    VkSampleCountFlagBits sampleCount;
    uint32_t mipLevel;
    uint32_t arrayLayers;
    VkImageTiling tiling;
    VkImageLayout initialLayout;

    constexpr static Self Default2D() {
        return {
                .sampleCount = VK_SAMPLE_COUNT_1_BIT,
                .mipLevel = 1,
                .arrayLayers = 1,
                .tiling = VK_IMAGE_TILING_OPTIMAL
        };
    }
};

constexpr inline VkImageType image_type(VkExtent3D extent) {
    VkImageType result{};

    uint32_t dim_num{0};

    if (extent.width >= 1) {
        dim_num++;
    }

    if (extent.height >= 1) {
        dim_num++;
    }

    if (extent.depth > 1) {
        dim_num++;
    }

    switch (dim_num) {
        case 1:
            result = VK_IMAGE_TYPE_1D;
            break;
        case 2:
            result = VK_IMAGE_TYPE_2D;
            break;
        case 3:
            result = VK_IMAGE_TYPE_3D;
            break;
        default:
            throw std::runtime_error("No image type found.");
    }

    return result;
}

constexpr inline bool is_depth_only_format(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM ||
           format == VK_FORMAT_D32_SFLOAT;
}

constexpr inline bool is_depth_stencil_format(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           is_depth_only_format(format);
}

class ImageView;

class Image : private NonCopyable {

    friend class ImageView;

    VkImage handle{};

    VmaAllocation memory;

    VkImageType type;

    VkExtent3D extent;

    VkFormat format;

    VkImageUsageFlags usage;

    VkSampleCountFlagBits sample_count;

    VkImageTiling tiling;

    VkImageSubresource subresource;

    Device *_device;

    std::unordered_set<ImageView *> views;

public:

    Image() = default;

    Image(Image &&other) noexcept;

    /// \brief bridge constructor, bridge vkImage to AN::VK::Image, it does not transfer the ownership
    Image(Device &          device,
          VkImage           handle,
          const VkExtent3D &extent,
          VkFormat          format,
          VkImageUsageFlags image_usage);

    ~Image();

    Image &operator = (Image &&other) noexcept {
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }



    bool init(Device &device, const ImageDescriptor &imageDescriptor);

    void deinit();

    Device &getDevice() const {
        return *_device;
    }

    VkImage vkImage() const {
        return handle;
    }

    VkImageType getType() const {
        return type;
    }

    const VkExtent3D &getExtent() const {
        return extent;
    }

    VkFormat getFormat() const {
        return format;
    }

    VkSampleCountFlagBits getSampleCount() const {
        return sample_count;
    }

    VkImageTiling getTiling() const {
        return tiling;
    }

    VkImageUsageFlags getUsage() const {
        return usage;
    }

    const VkImageSubresource &getSubresource() const {
        return subresource;
    }

    const std::unordered_set<ImageView *> &getViews() const {
        return views;
    }
};


class ImageView : private NonCopyable {

    Device *_device;

    Image *_image;

    VkImageView handle{};

    VkFormat _format;

    VkImageSubresourceRange subresourceRange;

    friend class Image;
public:

    ImageView() = default;

    ImageView(ImageView &&other) noexcept
        : _device(other._device), _image(other._image), handle(other.handle), _format(other._format),
          subresourceRange(other.subresourceRange) {
        other.handle = VK_NULL_HANDLE;

        _image->views.erase(&other);
        _image->views.emplace(this);
    }

    /// bridge constructor
    ImageView(Device &device, Image &image, VkImageView imageView, VkFormat format);

    ImageView &operator = (ImageView &&other) noexcept {
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    ~ImageView() {
        if (_image) {
            _image->views.erase(this);
        }
        deinit();
    }

    bool init(Image &image, VkImageViewType viewType, VkFormat format = VK_FORMAT_UNDEFINED);

    void deinit();

    Image &getImage() const {
        return *_image;
    }

    VkImageView vkImageView() const {
        return handle;
    }

    VkFormat getFormat() const {
        return _format;
    }

    const VkImageSubresourceRange &getSubresourceRange() const {
        return subresourceRange;
    }

    VkImageSubresourceLayers vkImageSubresourceLayers() const {
        VkImageSubresourceLayers subresource;
        subresource.aspectMask     = subresourceRange.aspectMask;
        subresource.baseArrayLayer = subresourceRange.baseArrayLayer;
        subresource.layerCount     = subresourceRange.layerCount;
        subresource.mipLevel       = subresourceRange.baseMipLevel;
        return subresource;
    }


};

}

#endif//OJOIE_VK_IMAGE_HPP
