//
// Created by aojoie on 9/20/2022.
//

#include "ojoie/Configuration/typedef.h"

#include "Render/PipelineReflection.hpp"
#include "Render/RenderPipelineState.hpp"
#include "Render/RenderTypes.hpp"
#include "Render/RenderContext.hpp"
#include "Render/private/D3D11/RenderPipelineState.hpp"

#ifdef OJOIE_USE_VULKAN
#include "./vulkan/RenderPipelineState.cpp"
#endif

namespace AN {

RenderPipelineState::RenderPipelineState() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        impl = new VK::RenderPipelineState();
#endif
    } else {
        impl = new D3D11::RenderPipelineState();
    }
}

RenderPipelineState::~RenderPipelineState() {
    delete impl;
    impl = nullptr;
}

void RenderPipelineState::deinit() {
    impl->deinit();
}

bool RenderPipelineState::init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                               const AN::PipelineReflection &reflection) {
    return impl->init(renderPipelineDescriptor, reflection);
}


}