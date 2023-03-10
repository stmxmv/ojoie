//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

#include <ojoie/Core/Window.hpp>

#include <ojoie/Render/CommandQueue.hpp>
#include <ojoie/Render/RenderCommandEncoder.hpp>
#include <ojoie/Render/BlitCommandEncoder.hpp>
#include <ojoie/Render/BufferManager.hpp>
#include <ojoie/Render/Device.hpp>

namespace AN {

namespace RC {

class Scene;


}
struct GraphicContext;

enum class AntiAliasingMethod {
    None = 0,
    MSAA,
    TAA
};

struct RenderContext {

    float deltaTime;
    float elapsedTime;

    float frameWidth, frameHeight;

    float windowWidth, windowHeight;

    float dpiScaleX, dpiScaleY;

    uint64_t frameCount;

    uint32_t maxFrameInFlight;

    uint32_t msaaSamples;

    AntiAliasingMethod antiAliasing;

    CursorState cursorState{ CursorState::Normal };

    Window *window;

    GraphicContext *graphicContext;

    bool forwardShading;
    mutable RC::Device device;
    mutable RC::BufferManager bufferManager;  /// current frame's bufferPool, always valid after renderer inited
    mutable RC::CommandQueue commandQueue; /// a commandQueue that its commandBuffer can be reset
    mutable RC::BlitCommandEncoder blitCommandEncoder;
    mutable RC::RenderCommandEncoder renderCommandEncoder; /// only valid in Node::render

    RC::Scene *scene;
};

}


#endif//OJOIE_RENDERCONTEXT_H
