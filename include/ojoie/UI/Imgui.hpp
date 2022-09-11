//
// Created by Aleudillonam on 9/10/2022.
//

#ifndef OJOIE_IMGUI_HPP
#define OJOIE_IMGUI_HPP

#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Render/Texture.hpp>
#include <ojoie/Render/Sampler.hpp>
#include <ojoie/Render/RenderPipeline.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/IndexBuffer.hpp>

namespace AN::UI {

class Imgui {

    uint64_t vertexBufferSize{};
    RC::VertexBuffer vertexBuffer;

    uint64_t indexBufferSize{};
    RC::IndexBuffer indexBuffer;

    RC::Texture fontTexture;
    RC::Sampler sampler;

    bool renderPipelineInited{};
    RC::RenderPipeline renderPipeline;

    std::vector<uint8_t> vertex_data;
    std::vector<uint8_t> index_data;

    void updateBuffer(RC::RenderCommandEncoder &renderCommandEncoder);

public:

    bool init();

    void deinit();

    void render(const RenderContext &context);

    void newFrame(const RenderContext &context);

    void endFrame(const RenderContext &context);

};

}

#endif//OJOIE_IMGUI_HPP
