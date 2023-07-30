//
// Created by aojoie on 4/20/2023.
//

#ifndef OJOIE_RENDERTARGET_HPP
#define OJOIE_RENDERTARGET_HPP

#include <ojoie/Render/Texture.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Core/CGTypes.hpp>

namespace AN {

struct RenderTargetImpl {
    virtual ~RenderTargetImpl() = default;

    virtual bool init(const RenderTargetDescriptor &attachmentDescriptor) = 0;

    virtual void deinit() = 0;

    virtual Size getSize() const = 0;

    virtual UInt32 getMSAASamples() const = 0;
};

class AN_API RenderTarget : public Texture {

    DECLARE_DERIVED_AN_CLASS(RenderTarget, Texture);

    bool bIsSwapchain:1;
    RenderTargetImpl *impl;

public:
    explicit RenderTarget(ObjectCreationMode mode);

    /// should not call this, unless you want to bridge swapchain image
    using Super::init;

    bool init(const RenderTargetDescriptor &attachmentDescriptor);

    bool init(const RenderTargetDescriptor &attachmentDescriptor, const SamplerDescriptor &samplerDescriptor);

    /// swapchain render target will also register its texture id
    /// this used when bridge a swapchain rendertarget
    /// you should call init() (no parameters) and then call this method any time you want
    void bridgeSwapchinRenderTargetInternal(RenderTargetImpl *impl);

    virtual void dealloc() override;

    Size getSize() const { return impl->getSize(); }

    UInt32 getMSAASamples() const { return impl->getMSAASamples(); }

    RenderTargetImpl *getImpl() const { return impl; }
};

}

#endif//OJOIE_RENDERTARGET_HPP
