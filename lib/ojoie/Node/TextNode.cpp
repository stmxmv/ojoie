//
// Created by Aleudillonam on 8/8/2022.
//

#include "Node/TextNode.h"
#include "Core/Game.hpp"
#include "Render/RenderQueue.hpp"
#include <ojoie/Render/Font.hpp>

namespace AN {


void TextNode::render(const RenderContext &context) {
    Super::render(context);

    float scale_value = std::min(r_scale.x, r_scale.y);

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

    GetFontManager().renderText(r_text.c_str(), r_color, font_width, font_edge, r_position.x, context.frameHeight - r_position.y, r_scale.x, r_scale.y);
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