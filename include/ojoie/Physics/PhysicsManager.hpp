//
// Created by Aleudillonam on 8/3/2023.
//

#pragma once

#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Render/CommandBuffer.hpp>

namespace AN {

class AN_API PhysicsManager {
    struct Impl;
    Impl *impl;

    bool m_Pause;

    PhysicsManager();

    ~PhysicsManager();
public:

    static PhysicsManager &GetSharedPhysics();

    bool init();

    void deinit();

    void update(float deltaTime);

    void pause(bool v);

    void clearState();

    void renderVisualization(CommandBuffer *commandBuffer);
};

static inline PhysicsManager &GetPhysicsManager() {
    return PhysicsManager::GetSharedPhysics();
}

}