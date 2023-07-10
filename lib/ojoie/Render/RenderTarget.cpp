//
// Created by aojoie on 4/20/2023.
//

#include "Render/RenderTarget.hpp"
#include "Render/RenderTypes.hpp"
#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/RenderTarget.hpp"
#include "Render/private/vulkan/TextureManager.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/Rendertarget.hpp"
#include "Render/private/D3D11/TextureManager.hpp"

namespace AN {

IMPLEMENT_AN_CLASS(RenderTarget);
LOAD_AN_CLASS(RenderTarget);

RenderTarget::~RenderTarget() {}
RenderTarget::RenderTarget(ObjectCreationMode mode) : Super(mode) {}

bool RenderTarget::init(const AttachmentDescriptor &attachmentDescriptor) {
    if (!Super::init()) return false;

    bIsSwapchain = false;

    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        VK::RenderTarget *target = new VK::RenderTarget();
        impl = target;

        if (!impl->init(attachmentDescriptor)) {
            return false;
        }

        VK::Texture tex{};
        tex.image = target->getImage();
        tex.imageView = target->getImageView();
        tex.samplerDescriptor = DefaultSamplerDescriptor();

        VK::GetTextureManager().registerRenderTarget(getTextureID(), tex);
#endif//OJOIE_USE_VULKAN
        return true;
    }

    if (GetGraphicsAPI() == kGraphicsAPID3D11) {
        D3D11::RenderTarget *target = new D3D11::RenderTarget();
        impl = target;

        if (!impl->init(attachmentDescriptor)) {
            return false;
        }

        /// register texture id
        D3D11::Texture tex{};
        tex._texture = target->getTexture();
        tex._srv = target->getSRV();
        tex.samplerDescriptor = DefaultSamplerDescriptor();

        D3D11::GetTextureManager().registerRenderTarget(getTextureID(), tex);
    }

    return true;
}

void RenderTarget::bridgeSwapchinRenderTargetInternal(RenderTargetImpl *implementation) {
    impl = implementation;
    bIsSwapchain = true;

    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        VK::RenderTarget *target = (VK::RenderTarget *)impl;
        VK::Texture tex{};
        tex.image = target->getImage();
        tex.imageView = target->getImageView();
        tex.samplerDescriptor = DefaultSamplerDescriptor();

        VK::GetTextureManager().registerRenderTarget(getTextureID(), tex);
#endif//OJOIE_USE_VULKAN
        return;
    }

    if (GetGraphicsAPI() == kGraphicsAPID3D11) {
        D3D11::RenderTarget *target = (D3D11::RenderTarget *)impl;

        /// register texture id
        /// register texture id
        D3D11::Texture tex{};
        tex._texture = target->getTexture();
        tex._srv = target->getSRV();
        tex.samplerDescriptor = DefaultSamplerDescriptor();

        D3D11::GetTextureManager().registerRenderTarget(getTextureID(), tex);

        return;
    }


}

void RenderTarget::dealloc() {
    if (impl && !bIsSwapchain) {
        impl->deinit();
        delete impl;
        impl = nullptr;
    }
    Super::dealloc();
}


}