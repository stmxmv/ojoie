//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

#include <ojoie/Core/Window.hpp>

#include <ojoie/Render/CommandBuffer.hpp>
#include <ojoie/Render/RenderCommandEncoder.hpp>

#include <ojoie/Render/Device.hpp>

namespace AN {

struct GraphicContext;

struct RenderContext {

    float deltaTime;
    float elapsedTime;

    float frameWidth, frameHeight;

    float windowWidth, windowHeight;

    float dpiScaleX, dpiScaleY;

    uint64_t frameCount;

    uint32_t maxFrameInFlight;

    uint32_t msaaSamples;

    CursorState cursorState{ CursorState::Normal };

    Window *window;

    GraphicContext *graphicContext;

    mutable RC::Device device;
    mutable RC::CommandBuffer commandBuffer;
    mutable RC::RenderCommandEncoder renderCommandEncoder;
};

}


#endif//OJOIE_RENDERCONTEXT_H
