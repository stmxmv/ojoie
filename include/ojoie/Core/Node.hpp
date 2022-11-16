//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_NODE_HPP
#define OJOIE_NODE_HPP

#include <memory>
#include <ojoie/Core/Dispatch.hpp>
#include <ojoie/Render/RenderContext.hpp>
#include <ojoie/Template/Iterator.hpp>
#include <ojoie/Render/SceneProxy.hpp>
#include <vector>
namespace AN {

namespace RC {
class Scene;
}
template<typename T>
concept node = requires (T node) {
                   { T::Alloc() } -> std::same_as<std::shared_ptr<T>>;
                   { node.init() } -> std::same_as<bool>;
               };

class Node : public std::enable_shared_from_this<Node> {
    typedef Node Self;
    friend class Game;
    friend class Renderer;
    friend class RC::Scene;
    friend class RC::SceneProxy;
    std::string _name;

protected:
    bool _needsRender;
    bool _postRender;
    bool tick;

    bool renderStateDirty{ true };

    std::weak_ptr<Node> _parent;

    std::vector<std::shared_ptr<Node>> _children;

    AN::RC::SceneProxy *sceneProxy;

public:

    struct NodeSceneProxy : public AN::RC::SceneProxy {


        explicit NodeSceneProxy(Node &node) : RC::SceneProxy(node) {
            needsRender = node._needsRender;
            needsPostRender = node._postRender;
        }

    };

    typedef NodeSceneProxy SceneProxyType;

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    explicit Node(bool render) : _needsRender(render), _postRender(), tick() {}
    Node() : Node(false) {}

    virtual ~Node() = default;

    virtual bool init();

    void addChild(const std::shared_ptr<Node> &child);

    auto parent() {
        return _parent.lock();
    }

    auto children() {

        struct s_Children : IndexedIteratorImpl<s_Children> {
            Node &self;

            uint64_t count() const {
                return self._children.size();
            }

            auto &objectAtIndex(uint64_t index) {
                return self._children[index];
            }

        } ret { {}, *this };

        return ret;
    }

    void setName(const char *name) {
        _name = name;
    }

    const char *getName() const {
        if (_name.empty()) {
#ifdef _MSC_VER
            const char *name =  typeid(*this).name() + 6;
#else
            const char *name = typeid(*this).name();
#endif
            return name;
        }
        return _name.c_str();
    }

    virtual void update(float deltaTime) {}

    virtual RC::SceneProxy *createSceneProxy() {
        return new NodeSceneProxy(*this);
    }

    RC::SceneProxy *getSceneProxy() const {
        return sceneProxy;
    }

    virtual void updateSceneProxy() {
        if (renderStateDirty) {

            struct Param {
                bool needsRender;
                bool postRender;
            } param{ .needsRender = _needsRender, .postRender = _postRender };

            sceneProxy->retain();
            Dispatch::async(Dispatch::Render, [param, sceneProxy = this->sceneProxy] {
                auto *proxy = (NodeSceneProxy *)(sceneProxy);
                proxy->setNeedsRender(param.needsRender);
                proxy->setNeedsPostRender(param.postRender);

                proxy->release();
            });

            renderStateDirty = false;
        }
    }

    /// \brief remove the node from parent
    void destroy();

    std::shared_ptr<Node> getRootNode() {
        std::shared_ptr<Node> root = shared_from_this();
        auto par = root->_parent.lock();
        while (par) {
            root = par;
            par = par->_parent.lock();
        }
        return root;
    }

    bool needsRender() const { return _needsRender; }

    bool isPostRender() const { return _postRender; }

    void setNeedsRender(bool value) {
        if (_needsRender != value) {
            _needsRender = value;
            renderStateDirty = true;
        }
    }

    void setPostRender(bool value) {
        if (_postRender != value) {
            _postRender = value;
            renderStateDirty = true;
        }
    }
};



class TestNode : public Node {
    typedef TestNode Self;
public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


};

static_assert(node<TestNode>);


}

#endif//OJOIE_NODE_HPP
