//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

#include <ojoie/Core/Window.hpp>

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
};

}


#endif//OJOIE_RENDERCONTEXT_H
