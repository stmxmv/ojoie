//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERER_HPP
#define OJOIE_RENDERER_HPP

#include <ojoie/Core/Node.hpp>

#include <ojoie/Render/RenderContext.hpp>

#include <ojoie/Core/Timer.hpp>
#include <ojoie/Core/delegate.hpp>
#include <ojoie/Render/RenderQueue.hpp>

namespace AN {

/// \brief a renderer manage rendering process of nodes in the render queue
class Renderer {
    std::atomic<Window *> currentWindow;

    /// RenderActor
    RenderContext renderContext{};
    std::vector<std::shared_ptr<Node>> nodesToRender;

    Renderer() = default;

    void renderOnce();

    friend class Window;
public:

    static Renderer &GetSharedRenderer();

    template<typename Func>
    void registerCleanupTask(Func &&func) {
        GetRenderQueue().registerCleanupTask(std::forward<Func>(func));
    }

    /// \attention must call in game thread
    void renderNodes(const std::vector<std::shared_ptr<Node>> &nodes);

    void render();

    Delegate<void()> completionHandler;

};


inline Renderer &GetRenderer() {
    return Renderer::GetSharedRenderer();
}


}

#endif//OJOIE_RENDERER_HPP
