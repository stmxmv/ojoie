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

namespace RC {
class Texture;
class UniformBuffer;
class Sampler;
class RenderPipeline;
}

/// \brief a renderer manage rendering process of nodes in the render queue
class Renderer {
    bool isStop{};

    /// \read AnyActor
    /// \write RenderActor
    std::atomic<Window *> currentWindow;

    /// \read RenderActor
    /// \write RenderActor
    CursorState currentCursorState{ CursorState::Normal };

    /// RenderActor
    RenderContext renderContext{};
    std::vector<std::shared_ptr<Node>> nodesToRender;
    std::vector<std::shared_ptr<Node>> postRenderNodes;

    friend class Window;

    struct Impl;
    Impl *impl;

    Renderer();

    ~Renderer();
public:

    static Renderer &GetSharedRenderer();


    bool init();

    void deinit();

    void willDeinit();

    void resourceFence();

    /// \GameActor
    void changeNodes(const std::vector<std::shared_ptr<Node>> &nodes);

    /// \GameActor
    void render(float deltaTime, float elapsedTime);

    Delegate<void()> completionHandler;

    /// \attention always consider renderCommandEncoder is invalid, which are valid in Node::render
    const RenderContext &getRenderContext() const {
        return renderContext;
    }

};


inline Renderer &GetRenderer() {
    return Renderer::GetSharedRenderer();
}

}

#endif//OJOIE_RENDERER_HPP
