//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERER_HPP
#define OJOIE_RENDERER_HPP

#include <ojoie/Core/Node.hpp>

#include <ojoie/Render/RenderContext.hpp>

#include <ojoie/Core/Timer.hpp>
#include <ojoie/Core/delegate.hpp>


namespace AN {

/// \brief a renderer manage rendering process of nodes in the render queue
class Renderer {
    int _maxFrameRate{ INT_MAX };
    RenderContext renderContext{};

//    Timer timer;

    std::vector<std::shared_ptr<Node>> nodesToRender;

    Renderer() = default;

    void renderOnce();

public:

    static Renderer &GetSharedRenderer();

    /// \attention must call in game thread
    void renderNodes(const std::vector<std::shared_ptr<Node>> &nodes);

    void render();

    void setMaxFrameRate(int rate) {
        _maxFrameRate = rate;
    }


    Delegate<void()> completionHandler;

};


inline Renderer &GetRenderer() {
    return Renderer::GetSharedRenderer();
}


}

#endif//OJOIE_RENDERER_HPP
