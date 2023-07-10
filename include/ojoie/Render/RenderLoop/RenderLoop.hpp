//
// Created by aojoie on 5/17/2023.
//

#ifndef OJOIE_RENDERLOOP_HPP
#define OJOIE_RENDERLOOP_HPP

#include <ojoie/Threads/Task.hpp>
#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Render/RenderTarget.hpp>

namespace AN {

typedef void (PerformRenderCallback)(RenderContext &context, void *userdata);

class RenderLoop {
public:

    virtual ~RenderLoop() = default;

    virtual bool init() = 0;

    virtual void deinit() = 0;

    virtual void setRenderTarget(RenderTarget *target) = 0;

    virtual void performUpdate(UInt32 frameVersion) = 0;

    virtual void performRender(RenderContext &context,
                               PerformRenderCallback renderCode,
                               void *userdata) = 0;
};


}

#endif//OJOIE_RENDERLOOP_HPP
