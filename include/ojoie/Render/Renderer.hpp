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

    const RenderContext &getRenderContext() const {
        return renderContext;
    }

    void didChangeRenderPipeline(class RC::RenderPipeline &pipeline);

    void bindUniformBuffer(uint32_t binding, class RC::UniformBuffer &uniformBuffer);

    void bindTexture(uint32_t binding, class RC::Texture &texture);

    void bindSampler(uint32_t binding, class RC::Sampler &sampler);

    void drawIndexed(uint32_t indexCount);

    void draw(uint32_t count);
};


inline Renderer &GetRenderer() {
    return Renderer::GetSharedRenderer();
}

namespace RC {

inline void DrawIndexed(uint32_t indexCount) {
    GetRenderer().drawIndexed(indexCount);
}

inline void Draw(uint32_t count) {
    GetRenderer().draw(count);
}

inline void BindUniformBuffer(uint32_t binding, class RC::UniformBuffer &uniformBuffer) {
    GetRenderer().bindUniformBuffer(binding, uniformBuffer);
}

inline void BindTexture(uint32_t binding, class RC::Texture &texture) {
    GetRenderer().bindTexture(binding, texture);
}

inline void BindSampler(uint32_t binding, class RC::Sampler &sampler) {
    GetRenderer().bindSampler(binding, sampler);
}


}


}

#endif//OJOIE_RENDERER_HPP
