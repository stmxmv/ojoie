//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

#include <ojoie/Core/Window.hpp>

namespace AN {

struct RenderContext {

    float deltaTime;
    float elapsedTime;

    float frameWidth, frameHeight;

    float windowWidth, windowHeight;

    float dpiScaleX, dpiScaleY;

    uint64_t frameCount;

    CursorState cursorState{ CursorState::Normal };

    Window *window;
};

}


#endif//OJOIE_RENDERCONTEXT_H
