//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_TEXTNODE_HPP
#define OJOIE_TEXTNODE_HPP

#include <ojoie/Node/Node2D.hpp>

namespace AN {


class TextNode : public Node2D {
    typedef TextNode Self;
    typedef Node2D Super;

    Math::vec2 _scale{ 1.f };
    std::string _text;
    Math::vec4  _color{ 1.f };

    bool dirty{};

public:

    struct TextNodeSceneProxy : Super::SceneProxyType {
        Math::vec2 scale;
        std::string text;
        Math::vec4  color;

        explicit TextNodeSceneProxy(TextNode &node) : Super::SceneProxyType(node) {
            scale = node._scale;
            text = node._text;
            color = node._color;
        }


        virtual void render(const RenderContext &context) override;
    };

    typedef TextNodeSceneProxy SceneProxyType;

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    TextNode() {
        _needsRender = true;
    }

    virtual bool init() override;

    virtual bool init(const char *text, const Math::vec4 &color);

    virtual RC::SceneProxy *createSceneProxy() override {
        return new TextNodeSceneProxy(*this);
    }

    virtual void updateSceneProxy() override {
        Super::updateSceneProxy();
        if (dirty) {

            struct Param {
                Math::vec2 scale;
                std::string text;
                Math::vec4  color;
            } param{ _scale, _text, _color };

            sceneProxy->retain();
            Dispatch::async(Dispatch::Render, [param, sceneProxy = sceneProxy]() mutable {
                auto *proxy = (TextNodeSceneProxy *)sceneProxy;

                proxy->scale = param.scale;
                proxy->text = param.text;
                proxy->color = param.color;

                proxy->release();
            });

            dirty = false;
        }
    }

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
        dirty = true;
    }

    void setColor(const Math::vec4 &color) {
        _color = color;
        dirty = true;
    }

    void setScale(float scale) {
        _scale.x = scale;
        _scale.y = scale;
        dirty = true;
    }

};



}

#endif//OJOIE_TEXTNODE_HPP
