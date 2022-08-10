//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_TEXTNODE_H
#define OJOIE_TEXTNODE_H

#include <ojoie/Node/Node2D.hpp>

namespace AN {


class TextNode : public Node2D {
    typedef TextNode Self;
    typedef Node2D Super;

    Math::vec2 _scale{ 1.f };
    std::string _text;
    Math::vec4  _color{ 1.f };

    Math::vec2 r_scale{ 1.f };
    std::string r_text;
    Math::vec4  r_color{ 1.f };
public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    TextNode() {
        _needsRender = true;
    }

    virtual bool init() override;

    virtual bool init(const char *text, const Math::vec4 &color);

    const char *getText() const {
        return _text.c_str();
    }

    const Math::vec4 &getColor() const {
        return _color;
    }

    const Math::vec2 &getScale() const {
        return _scale;
    }

    void setText(const char *text) {
        _text = text;
        Dispatch::async(Dispatch::Render, [text = std::string(text), weakSelf = weak_from_this()]() mutable {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *) _self.get();
                self->r_text = std::move(text);
            }
        });
    }

    void setColor(const Math::vec4 &color) {
        _color = color;
        Dispatch::async(Dispatch::Render, [color, weakSelf = weak_from_this()] {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *) _self.get();
                self->r_color = color;
            }
        });
    }

    void setScale(float scale) {
        _scale.x = scale;
        _scale.y = scale;
        Dispatch::async(Dispatch::Render, [scale, weakSelf = weak_from_this()] {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *) _self.get();
                self->r_scale.x = scale;
                self->r_scale.y = scale;
            }
        });
    }

    virtual void render(const RenderContext &context) override;
};



}

#endif//OJOIE_TEXTNODE_H
