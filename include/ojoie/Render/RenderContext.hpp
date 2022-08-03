//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERCONTEXT_H
#define OJOIE_RENDERCONTEXT_H

namespace AN {

class Window;

struct RenderContext {

    float deltaTime;
    float elapsedTime;

    Window *window;
};

}


#endif//OJOIE_RENDERCONTEXT_H
