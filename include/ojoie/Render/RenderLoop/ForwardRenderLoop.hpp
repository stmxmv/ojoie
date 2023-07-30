//
// Created by aojoie on 4/23/2023.
//

#ifndef OJOIE_FORWARDRENDERLOOP_HPP
#define OJOIE_FORWARDRENDERLOOP_HPP

#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Render/RenderPass.hpp>
#include <ojoie/Render/RenderLoop/RenderLoop.hpp>
#include <ojoie/Threads/Task.hpp>
#include <ojoie/Object/ObjectPtr.hpp>

namespace AN {

class AN_API ForwardRenderLoop : public RenderLoop {

    RenderTarget *renderTarget;
    ObjectPtr<RenderTarget> depthTexture;

    Size _renderArea;
    UInt32 _msaaSamples;

    bool createRenderPass();

    void recreateAttachments(const Size &size, UInt32 msaaSamples);

public:

    ForwardRenderLoop();

    ~ForwardRenderLoop();

    virtual bool init() override;

    virtual void deinit() override;

    virtual void setRenderTarget(RenderTarget *target) override;

    /// update in game thread
    virtual void performUpdate(UInt32 frameVersion) override;

    /// completionHandler with no return or parameter just use task
    virtual void performRender(RenderContext &context,
                               PerformRenderCallback renderCode,
                               void *userdatar) override;

};


}// namespace AN

#endif//OJOIE_FORWARDRENDERLOOP_HPP
