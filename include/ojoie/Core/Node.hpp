//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_NODE_HPP
#define OJOIE_NODE_HPP

#include <memory>
#include <ojoie/Core/Dispatch.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <vector>
namespace AN {

template<typename T>
concept node = requires (T node) {
                   { T::Alloc() } -> std::same_as<std::shared_ptr<T>>;
                   { node.init() } -> std::same_as<bool>;
               };

class Node : public std::enable_shared_from_this<Node> {
    typedef Node Self;
    friend class Game;
    friend class Renderer;
protected:
    bool _needsRender;
    bool r_needsRender;
    bool tick;
    std::weak_ptr<Node> parent;

    std::vector<std::shared_ptr<Node>> _children;

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    explicit Node(bool render) : _needsRender(render), tick() {}
    Node() : Node(false) {}

    virtual ~Node() = default;

    virtual bool init();

    void addChild(const std::shared_ptr<Node> &child);

    virtual void update(float deltaTime) {}

    /// \brief called in the render queue if canRender is true
    virtual void render(const struct RenderContext &context) {}

    /// \brief remove the node from parent
    void destroy();

    std::shared_ptr<Node> getRootNode() {
        std::shared_ptr<Node> root = shared_from_this();
        auto par = root->parent.lock();
        while (par) {
            root = par;
            par = par->parent.lock();
        }
        return root;
    }

    bool needsRender() const { return _needsRender; }

    void setNeedsRender(bool value) {
        if (_needsRender != value) {
            _needsRender = value;
            Dispatch::async(Dispatch::Render, [value, _self = weak_from_this()] {
                auto self = _self.lock();
                if (self) {
                    self->r_needsRender = value;
                }
            });
        }
    }
};



class TestNode : public Node {
    typedef TestNode Self;
public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual void render(const struct RenderContext &context) override {

    }
};

static_assert(node<TestNode>);


}

#endif//OJOIE_NODE_HPP
