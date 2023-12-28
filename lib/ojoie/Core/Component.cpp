//
// Created by aojoie on 4/1/2023.
//

#include "Core/Component.hpp"
#include "Core/Actor.hpp"

namespace AN {

IMPLEMENT_AN_CLASS(Component)
LOAD_AN_CLASS(Component)
IMPLEMENT_AN_OBJECT_SERIALIZE(Component)
INSTANTIATE_TEMPLATE_TRANSFER(Component)

Component::Component(ObjectCreationMode mode) : Super(mode), _actor(), m_IsDestroying() {}

Component::~Component() {

}


bool Component::isActive() const {
    Actor *actor = _actor;
    return actor != nullptr && actor->isActive();
}

void Component::dealloc() {

    m_IsDestroying = true;

    Message message;
    message.sender = getActorPtr(); /// match addComponent, sender is actor
    message.name = kWillRemoveComponentMessage;
    message.data = (intptr_t)this;
    getActor().sendMessage(message);

    for (auto it = _actor->getComponentsContainer().begin(); it != _actor->getComponentsContainer().end(); ++it) {
        if (it->second == this) {
            _actor->getComponentsContainer().erase(it);
            break;
        }
    }

    m_IsDestroying = false;
    Super::dealloc();
}

template<typename _Coder>
void Component::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(_actor);
}

}