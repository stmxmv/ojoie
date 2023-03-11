//
// Created by aojoie on 10/23/2022.
//

#ifndef OJOIE_SCENE_HPP
#define OJOIE_SCENE_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/Render/RenderQueue.hpp>
#include <ojoie/Render/SceneProxy.hpp>
#include <ojoie/Template/RC.hpp>

namespace AN::RC {


class SceneProxyInfo : public ReferenceCounted<SceneProxyInfo> {
public:
    std::shared_ptr<Node> node;
    SceneProxy *sceneProxy{};
    uint64_t packedIndex{};
};


class Scene {

    std::vector<SceneProxyInfo *> sceneProxyInfos;
    std::vector<SceneProxyInfo *> addedSceneProxyInfos;
    std::vector<SceneProxyInfo *> removedSceneProxyInfos;

    Math::mat4 preViewMatrix{ 1.f }, preProjectionMatrix{ 1.f };
    Math::mat4 viewMatrix, projectionMatrix;

public:

    void deinit() {
        clearNodes();
    }

    void addNode(const std::shared_ptr<Node> &node) {
        SceneProxy *proxy = node->createSceneProxy();

        if (!proxy) {
            return;
        }

        SceneProxyInfo *info = new SceneProxyInfo();
        info->node = node;
        info->sceneProxy = proxy;
        info->sceneProxy->sceneProxyInfo = info;

        info->sceneProxy->scene = this;
        info->sceneProxy->retain();

        GetRenderQueue().enqueue([info, this] {

            if (!info->sceneProxy->createRenderResources()) {
                info->sceneProxy->release();
                info->sceneProxy->release(); // extra release for lambda block
                info->release();
                return;
            }

            info->node->sceneProxy = info->sceneProxy;

            addedSceneProxyInfos.push_back(info);

            info->sceneProxy->release();
        });
    }

    void removeNode(const std::shared_ptr<Node> &node) {
        GetRenderQueue().enqueue([node, this] {

            if (std::erase(addedSceneProxyInfos, node->sceneProxy->sceneProxyInfo)) {
                removedSceneProxyInfos.push_back(node->sceneProxy->sceneProxyInfo);

                node->sceneProxy->destroyRenderResources();
                node->sceneProxy->sceneProxyInfo->release();
                node->sceneProxy->release();

                node->sceneProxy = nullptr;

            } else {

                removedSceneProxyInfos.push_back(node->sceneProxy->sceneProxyInfo);

            }
        });
    }

    void clearNodes() {
        GetRenderQueue().enqueue([this] {

            for (auto &info : sceneProxyInfos) {
                info->sceneProxy->destroyRenderResources();
                info->sceneProxy->release();
                info->node->sceneProxy = nullptr;

                info->release();
            }

            sceneProxyInfos.clear();
        });
    }

    /// \GameActor
    void updateSceneProxies() {

        {
            if (!removedSceneProxyInfos.empty()) {
                uint32_t removedNum = removedSceneProxyInfos.size();
                uint32_t destIndex = sceneProxyInfos.size() - 1;

                for (auto *info : removedSceneProxyInfos) {
                    info->sceneProxy->sceneProxyInfo->release();
                    info->sceneProxy->release();
                    info->node->sceneProxy = nullptr;

                    uint32_t sourceIndex = info->packedIndex;

                    if (sourceIndex != destIndex--) {
                        std::swap(sceneProxyInfos[sourceIndex], sceneProxyInfos[destIndex]);
                    }
                }
                removedSceneProxyInfos.clear();
                sceneProxyInfos.erase(sceneProxyInfos.end() - removedNum, sceneProxyInfos.end());
            }
        }

        {
            if (!addedSceneProxyInfos.empty()) {
                uint32_t addNum = addedSceneProxyInfos.size();
                sceneProxyInfos.reserve(sceneProxyInfos.size() + addNum);
                for (auto *info : addedSceneProxyInfos) {
                    sceneProxyInfos.push_back(info);
                    info->packedIndex = sceneProxyInfos.size() - 1;
                }
                addedSceneProxyInfos.clear();
            }
        }
    }

    /// \RenderActor
    void doRender(const struct RenderContext &context) {
        for (auto &info : sceneProxyInfos) {
            if (info->sceneProxy->needsRender) {
                info->sceneProxy->render(context);
            }
        }
    }

    /// \RenderActor
    void doPostRender(const struct RenderContext &context) {
        for (auto &info : sceneProxyInfos) {
            if (info->sceneProxy->needsPostRender) {
                info->sceneProxy->postRender(context);
            }
        }
    }

    /// \RenderActor
    void setViewMatrix(const Math::mat4 &matrix) {
        preViewMatrix = viewMatrix;
        viewMatrix = matrix;
    }

    /// \RenderActor
    void setProjectionMatrix(const Math::mat4 &matrix) {
        preProjectionMatrix = projectionMatrix;
        projectionMatrix = matrix;
    }

    /// \RenderActor
    const Math::mat4 &getViewMatrix() const {
        return viewMatrix;
    }

    /// \RenderActor
    const Math::mat4 &getProjectionMatrix() const {
        return projectionMatrix;
    }

    /// \RenderActor
    const Math::mat4 &getPreViewMatrix() const {
        return preViewMatrix;
    }

    /// \RenderActor
    const Math::mat4 &getPreProjectionMatrix() const {
        return preProjectionMatrix;
    }
};


}

#endif//OJOIE_SCENE_HPP
