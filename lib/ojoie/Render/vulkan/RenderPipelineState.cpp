//
// Created by aojoie on 9/20/2022.
//

#include "Render/RenderPipelineState.hpp"
#include "Render/private/vulkan/RenderPipelineState.hpp"

namespace AN::VK {




}

namespace {

}

namespace AN::RC {

RenderPipelineState::RenderPipelineState() {
    impl = new VK::RenderPipelineState();
}

RenderPipelineState::~RenderPipelineState() {
    delete (VK::RenderPipelineState *)impl;
    impl = nullptr;
}

void RenderPipelineState::deinit() {
    VK::RenderPipelineState *self = (VK::RenderPipelineState *)impl;
    self->deinit();
}

bool RenderPipelineState::init(const AN::RC::RenderPipelineStateDescriptor &renderPipelineDescriptor) {
    VK::RenderPipelineState *self = (VK::RenderPipelineState *)impl;
    return self->init(renderPipelineDescriptor);
}

}