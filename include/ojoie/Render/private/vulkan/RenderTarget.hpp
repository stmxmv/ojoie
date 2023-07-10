//
// Created by Aleudillonam on 9/3/2022.
//

#ifndef OJOIE_VK_RENDERTARGET_HPP
#define OJOIE_VK_RENDERTARGET_HPP

#include "ojoie/Render/private/vulkan/Image.hpp"
#include <ojoie/Render/RenderTarget.hpp>

namespace AN::VK {


class RenderTarget : public AN::RenderTargetImpl, public NonCopyable {
    VkExtent2D            _extent;
    VkImage               _image;
    VmaAllocation         _imageAllocation;
    VkImageView           _imageView;
    VkFormat              _format;
    VkSampleCountFlagBits _samples;
    VkImageCreateFlags    _imageCreateFlags;
    VkImageUsageFlags     _imageUsage;
    bool                  bIsSwapchain : 1;

public:
    RenderTarget();

    RenderTarget(RenderTarget &&other) noexcept;

    virtual ~RenderTarget() override {
        deinit();
    }

    bool init(VkFormat              format,
              const VkExtent2D     &extent,
              VkSampleCountFlagBits samples);

    virtual bool init(const AttachmentDescriptor &attachmentDescriptor) override;

    virtual void deinit() override;

    virtual Size getSize() const override { return { .width = _extent.width, .height = _extent.height }; }

    /// note that if target is swapchain image, cannot call this method,
    /// you must acquire the new image and bridge it
    void resize(const VkExtent2D &extent);

    /// init swapchain render target that render on screen
    void bridgeSwapchainRenderTarget(VkImage           image,
                                     const VkExtent2D &extent,
                                     VkFormat          format,
                                     VkImageCreateFlags    imageCreateFlags,
                                     VkImageUsageFlags     imageUsage);

    VkFormat getFormat() const { return _format; }

    VkImage getImage() const { return _image; }

    VkImageView getImageView() const { return _imageView; }

    const VkExtent2D &getExtent() const { return _extent; }

    VkSampleCountFlagBits getSamples() const { return _samples; }

    VkImageCreateFlags getImageCreateFlags() const { return _imageCreateFlags; }
    VkImageUsageFlags getImageUsage() const { return _imageUsage; }

    /// currently usage and flags will not exposed
    static VkImageCreateFlags GetImageCreateFlags(VkFormat format);
    static VkImageUsageFlags GetImageUsageFlags(VkFormat format);
};


}// namespace AN::VK

#endif//OJOIE_VK_RENDERTARGET_HPP
