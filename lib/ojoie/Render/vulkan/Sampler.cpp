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


bool Sampler::init(const SamplerDescriptor &samplerDescriptor) {
    const RenderContext &context = GetRenderer().getRenderContext();
    impl->device = context.graphicContext->device;


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