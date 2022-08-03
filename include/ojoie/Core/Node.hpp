//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_NODE_HPP
#define OJOIE_NODE_HPP

#include <ojoie/Render/RenderContext.hpp>

#include <memory>
#include <vector>
#include <unordered_set>
#include <memory>

namespace AN {

template<typename T>
concept node = requires (T node) {
                   { T::Alloc() } -> std::same_as<std::shared_ptr<T>>;
                   { node.init() } -> std::same_as<bool>;
               };

class Node : public std::enable_shared_from_this<Node> {
    typedef Node Self;
    friend class Game;
protected:
    bool canRender;
    bool tick;
    std::weak_ptr<Node> parent;

    std::unordered_set<std::shared_ptr<Node>> _children;

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    explicit Node(bool render) : canRender(render), tick() {}
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
