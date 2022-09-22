//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_IMGUI_HPP
#define OJOIE_IMGUI_HPP

#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Render/Sampler.hpp>
#include <ojoie/Render/RenderPipelineState.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/IndexBuffer.hpp>

namespace AN::UI {

class Imgui {

    RC::Texture fontTexture;
    RC::Sampler sampler;

    bool renderPipelineInited{};
    RC::RenderPipelineState renderPipelineState;

    std::vector<uint8_t> vertex_data;
    std::vector<uint8_t> index_data;

    void updateBuffer(const RenderContext &context, RC::RenderCommandEncoder &renderCommandEncoder);

public:

    bool init();

    void deinit();

    void render(const RenderContext &context);

    void newFrame(const RenderContext &context);

    void endFrame(const RenderContext &context);

};

}

#endif//OJOIE_IMGUI_HPP
