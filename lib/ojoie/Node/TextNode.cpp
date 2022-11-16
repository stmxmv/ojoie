//
// Created by Aleudillonam on 8/8/2022.
//

#include "Node/TextNode.hpp"
#include "Core/Game.hpp"
#include "Render/RenderQueue.hpp"
#include <ojoie/Render/Font.hpp>

namespace AN {



bool TextNode::init() {
    if (Super::init()) {

        return true;
    }
    return false;
}


bool TextNode::init(const char *text, const glm::vec4 &color) {
    if (Self::init()) {
        _text = text;
        _color = color;
        return true;
    }


    return false;
}

void TextNode::TextNodeSceneProxy::render(const RenderContext &context) {
    Super::SceneProxyType::render(context);

    float scale_value = std::min(scale.x, scale.y);

    float font_width = 0.5f;
    float font_edge = 0.1f;

    if (scale_value > 1.f) {
        scale_value = 1.f / scale_value;
        font_width += 0.03f * scale_value;
        font_edge -= 0.25f * scale_value;
    } else {
        font_width -= 0.15f * scale_value;
        font_edge += 0.15f * scale_value;
    }

    GetFontManager().renderText(text.c_str(), color, font_width, font_edge, position.x,
                                context.frameHeight - position.y, scale.x, scale.y);

}
}