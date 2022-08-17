//
// Created by Aleudillonam on 8/8/2022.
//

#include "Node/TextNode.h"
#include "Core/Game.hpp"
#include "Render/RenderQueue.hpp"
#include <ojoie/Render/Font.hpp>

namespace AN {


static const char *vertexSource = "#version 430 core\n"
                                  "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
                                  "out vec2 TexCoords;\n"
                                  "\n"
                                  "uniform mat4 projection;\n"
                                  "\n"
                                  "void main()\n"
                                  "{\n"
                                  "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
                                  "    TexCoords = vertex.zw;\n"
                                  "}";

static const char *framentSource = "#version 430 core\n"
                                   "in vec2 TexCoords;\n"
                                   "out vec4 color;\n"
                                   "\n"
                                   "uniform sampler2D text;\n"
                                   "uniform vec4 textColor;\n"
                                   "\n"
                                   "uniform float width;\n"
                                   "uniform float edge;\n"
                                   "\n"
                                   "void main() {\n"
                                   "\n"
                                   "    float distance = 1.f - texture(text, TexCoords).r;\n"
                                   "    float alpha = 1.f - smoothstep(width, width + edge, distance);\n"
                                   "\n"
                                   "    color = vec4(textColor.rgb, textColor.a * alpha);\n"
                                   "}";

static RC::RenderPipeline pipeline;


static Math::mat4 projectionMatrix;

void TextNode::render(const RenderContext &context) {
    Super::render(context);
    static bool pipelineInited = false;
    if (!pipelineInited) {
        pipelineInited = true;
//        ANAssert(pipeline.initWithSource(vertexSource, framentSource));
        GetRenderQueue().registerCleanupTask([] {
            pipeline.deinit();
        });
    }

    static uint64_t lastFrame = -1;
    static float width = 0, height = 0;
    if (lastFrame != context.frameCount) {
        lastFrame = context.frameCount;

        if (width != context.frameWidth || height != context.frameHeight) {
            width = context.frameWidth;
            height = context.frameHeight;
            projectionMatrix = Math::ortho(0.0f, width, 0.f, height, -1.0f, 1.0f);
        }
    }
//
//    pipeline.activate();
//
//    pipeline.setVec4("textColor", r_color);
//    pipeline.setMat4("projection", projectionMatrix);

    float scale_value = std::min(r_scale.x, r_scale.y);

    float font_width = 0.5f;
    float font_edge = 0.1f;

    if (scale_value > 1.f) {
        scale_value = 1.f / scale_value;
        font_width += 0.05f * scale_value;
        font_edge -= 0.15f * scale_value;
    } else {
        font_width -= 0.1f * scale_value;
        font_edge += 0.15f * scale_value;
    }

//    pipeline.setFloat("width", font_width);
//    pipeline.setFloat("edge", font_edge);

    GetFontManager().renderText(pipeline, r_text.c_str(), r_position.x, height - r_position.y, r_scale.x, r_scale.y);

}

bool TextNode::init() {
    if (Super::init()) {


        return true;
    }
    return false;
}


bool TextNode::init(const char *text, const glm::vec4 &color) {
    if (Self::init()) {
        _text = text;
        r_text = text;
        _color = color;
        r_color = color;

        return true;
    }


    return false;
}

}