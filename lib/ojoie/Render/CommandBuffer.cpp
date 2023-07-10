//
// Created by Aleudillonam on 9/10/2022.
//

#include "ojoie/Render/CommandBuffer.hpp"

#ifdef OJOIE_USE_VULKAN
#include "./vulkan/CommandBuffer.cpp"
#endif

namespace AN {

static bool gEmitDebugLabel = false;

void CommandBuffer::SetEmitDebugLabel(bool emit) {
    gEmitDebugLabel = emit;
}

bool CommandBuffer::IsEmittingDebugLabel() {
    return gEmitDebugLabel;
}

}
