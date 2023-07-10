//
// Created by aojoie on 3/31/2023.
//

#include "Core/Actor.hpp"
#include "Utility/Log.h"
#include "Template/Access.hpp"
#include <algorithm>
#include <ranges>

namespace AN {

AN_API const Name kDidAddComponentMessage("DidAddComponent");

IMPLEMENT_AN_CLASS(Actor);
LOAD_AN_CLASS(Actor);

Actor::Actor(ObjectCreationMode mode)
    : Super(mode), bIsActive(), bIsActivating(), bIsDestroying() {}


bool Actor::init(std::span<int> components) {
    if (!init()) return false;
    for (int com : components) {
        addComponentInternal(com);
    }
    return true;
}

bool Actor::init() {
    if (Super::init()) {
        setName("Actor");
        bIsActive = true; // actor init as active

        /// add default components
        addComponent<TransformComponent>();

        return true;
    }
    return false;
}

bool Actor::init(Name name, std::span<int> components) {
    if (!init(components)) {
        return false;
    }
    setName(name);
    return true;
}

bool Actor::init(Name name) {
    if (!init()) return false;
    setName(name);
    return true;
}

bool Actor::isActive() const {
    return bIsActive;
}

void Actor::destroy() {
    willDestroyActor();
    GetActorManager().removeActor(_actorListNode);
    DestroyObject(this);
}

void Actor::willDestroyActor() {
    bIsDestroying = true;
    for (auto &[id, comPtr] : components) {
        if (comPtr->isActive()) {
            comPtr->deactivate();
        }
        comPtr->willDestroyComponent();
    }
}

void Actor::activate() {
    if (isActive()) {
        return;
    }

    // TODO activate recursively

    bIsActive = true;
    bIsActivating = true;
    for (auto &[id, comPtr] : components) {
        comPtr->activate();
    }
    bIsActivating = false;
}

void Actor::deactivate() {
    if (!isActive()) return;
    // TODO set deactivate recursively

    bIsActive = false;

    for (auto &[id, comPtr] : components) {
        comPtr->deactivate();
    }
}

Component *Actor::addComponentInternal(int id) {
    Class *cls = Class::GetClass(id);
    if (!cls) {
        ANLog("Component class id %d not exist", id);
        return nullptr;
    }
    if (!cls->isDerivedFrom<Component>()) {
        ANLog("cannot add component class id %d name %s which is not a subclass of Component", id, cls->getClassName());
        return nullptr;
    }
    ObjectPtr<Component> componentPtr = MakeObjectPtr<Component>(id);
    ANAssert(componentPtr != nullptr); // should be success
    componentPtr->setActorInternal(this); // set actor first because init may add another component
    componentPtr->init();


    components.emplace_back(id, std::move(componentPtr));

    Message message;
    message.sender = this;
    message.data = (intptr_t)components.back().second.get();
    message.name = kDidAddComponentMessage;
    sendMessage(message);

    if (!isActive()) {
        components.back().second->deactivate();
    }

    return components.back().second.get();
}

Component *Actor::getComponentExactClassInternal(int inID) {
    for (auto &[id, comPtr] : components) {
        if (id == inID) {
            return comPtr.get()->asUnsafe<Component>();
        }
    }
    return nullptr;
}

Component *Actor::getComponentInternal(int inID) {
    /// find the first component of class id
    for (auto &[id, comPtr] : components) {
        if (id == inID || comPtr->isDerivedFrom(inID)) {
            return comPtr.get()->asUnsafe<Component>();
        }
    }
    return nullptr;
}

Actor::~Actor() {

}

void Actor::sendMessage(Message &message) {
    if (Super::respondToMessage(message.name)) {
        Super::sendMessage(message);
    }

    for (auto &com : components | std::views::values) {
        if (com->respondToMessage(message.name)) {
            com->sendMessage(message);
        }
    }
}

bool Actor::respondToMessage(MessageName name) {
    if (Super::respondToMessage(name)) {
        return true;
    }

    for (auto &com : components | std::views::values) {
        if (com->respondToMessage(name)) {
            return true;
        }
    }
    return false;
}

void ActorManager::addActor(ActorListNode &node) {
    _actorList.push_back(node);
}

void ActorManager::removeActor(ActorListNode &node) {
    node.removeFromList();
}

ActorManager &GetActorManager() {
    static ActorManager actorManager;
    return actorManager;
}

}