//
// Created by aojoie on 5/24/2023.
//

#pragma once

#include <ojoie/Render/RenderPass.hpp>

namespace AN::D3D11 {

struct RenderPass : public AN::RenderPassImpl {

    RenderPassDescriptor renderPassDescriptor;

    virtual bool init(const RenderPassDescriptor &aRenderPassDescriptor) override {
        renderPassDescriptor = aRenderPassDescriptor;
        return true;
    }

    virtual void deinit() override {}
};

}
