//
// Created by aojoie on 10/23/2022.
//

#ifndef OJOIE_SCENEPROXY_HPP
#define OJOIE_SCENEPROXY_HPP

#include <ojoie/Template/RC.hpp>

namespace AN {
class Node;

}
namespace AN::RC {

class Scene;
class SceneProxyInfo;

class SceneProxy : public ReferenceCounted<SceneProxy> {
    friend class Scene;
protected:

    bool needsRender;
    bool needsPostRender;

    Node &owner;
    Scene *scene;
    SceneProxyInfo *sceneProxyInfo;

public:

    explicit SceneProxy(Node &node) : owner(node) {}

    virtual ~SceneProxy() = default;

    virtual bool createRenderResources() { return true; }

    virtual void destroyRenderResources() {}

    virtual void render(const struct RenderContext &context) {}

    virtual void postRender(const struct RenderContext &context) {}

    Scene &getScene() const {
        return *scene;
    }

    void setNeedsRender(bool aNeedsRender) {
        needsRender = aNeedsRender;
    }

    void setNeedsPostRender(bool aNeedsPostRender) {
        needsPostRender = aNeedsPostRender;
    }
};

}

#endif//OJOIE_SCENEPROXY_HPP
