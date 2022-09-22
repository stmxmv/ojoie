//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/Sampler.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"

#include "Render/private/vulkan/Device.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace AN::RC {

struct Sampler::Impl {
    VK::Device *device;
    VkSampler sampler;
};

Sampler::Sampler() : impl(new Impl{}) {}

Sampler::~Sampler() {
    deinit();
    delete impl;
}

static VkSamplerAddressMode toVkSamplerAddressMode(SamplerAddressMode addressMode) {
    switch (addressMode) {
        case SamplerAddressMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::MirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case SamplerAddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::ClampToBorderColor:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case SamplerAddressMode::MirrorClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        case SamplerAddressMode::ClampToZero:
            ANLog("Not support SamplerAddressMode ClampToZero in Vulkan, use Repeat instead");
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    throw Exception("Invalid enum value");
}

static VkBorderColor toVkBorderColor(SamplerBorderColor borderColor) {
    switch (borderColor) {
        case SamplerBorderColor::TransparentBlack:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case SamplerBorderColor::OpaqueBlack:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case SamplerBorderColor::OpaqueWhite:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }
    throw Exception("Invalid enum value");
}

static VkFilter toVkFilter(SamplerMinMagFilter filter) {
    switch (filter) {
        case SamplerMinMagFilter::Nearest:
            return VK_FILTER_NEAREST;
        case SamplerMinMagFilter::Linear:
            return VK_FILTER_LINEAR;
    }
    throw Exception("Invalid enum value");
}

static VkSamplerMipmapMode toVkSamplerMipmapMode(SamplerMipFilter mipFilter) {
    switch (mipFilter) {
        case SamplerMipFilter::Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case SamplerMipFilter::Linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case SamplerMipFilter::NotMipmapped:
            ANLog("Not support SamplerMipFilter NotMipmapped in Vulkan, use Linear instead");
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    throw Exception("Invalid enum value");
}

static VkCompareOp toVkCompareOp(CompareFunction function) {
    switch (function) {
        case CompareFunction::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareFunction::Less:
            return VK_COMPARE_OP_LESS;
        case CompareFunction::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareFunction::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareFunction::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareFunction::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareFunction::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareFunction::Always:
            return VK_COMPARE_OP_ALWAYS;
    }
    throw Exception("Invalid enum value");
}

bool Sampler::init(const SamplerDescriptor &samplerDescriptor) {
    const RenderContext &context = GetRenderer().getRenderContext();
    impl->device = context.graphicContext->device;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.unnormalizedCoordinates = !samplerDescriptor.normalizedCoordinates;
    samplerInfo.addressModeU = toVkSamplerAddressMode(samplerDescriptor.addressModeU);
    samplerInfo.addressModeV = toVkSamplerAddressMode(samplerDescriptor.addressModeV);
    samplerInfo.addressModeW = toVkSamplerAddressMode(samplerDescriptor.addressModeW);
    samplerInfo.borderColor = toVkBorderColor(samplerDescriptor.borderColor);
    samplerInfo.minFilter = toVkFilter(samplerDescriptor.minFilter);
    samplerInfo.magFilter = toVkFilter(samplerDescriptor.magFilter);
    samplerInfo.mipmapMode = toVkSamplerMipmapMode(samplerDescriptor.mipFilter);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = (float)samplerDescriptor.maxAnisotropy;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = toVkCompareOp(samplerDescriptor.compareFunction);
    samplerInfo.minLod = samplerDescriptor.lodMinClamp;
    samplerInfo.maxLod = samplerDescriptor.lodMaxClamp;
    samplerInfo.mipLodBias = 0.f;

    if (vkCreateSampler(impl->device->vkDevice(), &samplerInfo, nullptr, &impl->sampler) != VK_SUCCESS) {
        ANLog("failed to create texture sampler!");
        return false;
    }

    return true;
}

void Sampler::deinit() {
    if (impl && impl->sampler) {
        vkDestroySampler(impl->device->vkDevice(), impl->sampler, nullptr);
        impl->sampler = nullptr;
    }
}


void *Sampler::getUnderlyingSampler() const {
    return (void *)impl->sampler;
}


}