//
// Created by aojoie on 4/1/2023.
//

#include "Core/Component.hpp"
#include "Core/Actor.hpp"

namespace AN {

IMPLEMENT_AN_CLASS(Component);
LOAD_AN_CLASS(Component);

Component::~Component() {

}


bool Component::isActive() const {
    Actor *actor = _actor;
    return actor != nullptr && actor->isActive();
}

void Component::dealloc() {
    for (auto it = _actor->getComponents().begin(); it != _actor->getComponents().end(); ++it) {
        if (it->second == this) {
            _actor->getComponents().erase(it);
            break;
        }
    }
    Super::dealloc();
}

}