//
// Created by aojoie on 3/31/2023.
//

#ifndef OJOIE_ACTOR_HPP
#define OJOIE_ACTOR_HPP

#include <ojoie/Components/Transform.hpp>
#include <ojoie/Configuration/typedef.h>
#include <ojoie/Core/Component.hpp>
#include <ojoie/Object/NamedObject.hpp>
#include <ojoie/Object/ObjectPtr.hpp>
#include <ojoie/Template/LinkedList.hpp>
#include <span>
#include <vector>

namespace AN {

/// data contain added component
AN_API extern const Name kDidAddComponentMessage;

AN_API extern const Name kWillRemoveComponentMessage;

class Actor;

typedef ListNode<Actor> ActorListNode;

typedef List<ActorListNode> ActorList;

class AN_API Actor final : public NamedObject {

    typedef std::vector<std::pair<int, Component *>> ComponentContainer;
    bool bIsActive:1;
    bool bIsActivating:1;
    bool bIsDestroying:1;
    ComponentContainer  components;

    ActorListNode _actorListNode{ this };

    DECLARE_DERIVED_AN_CLASS(Actor, NamedObject);

    static bool IsSealedClass() { return true; }

public:

    explicit Actor(ObjectCreationMode mode);

    /// unlike Object, sendMessage to Actor, will send to its components those can receive that message
    virtual void sendMessage(Message &message) override;
    virtual bool respondToMessage(MessageName name) override;

    virtual bool init() override;
    bool init(std::span<int> components);
    bool init(Name name, std::span<int> components);
    virtual bool init(Name name) override;

    bool isDestroying() const { return bIsDestroying; }

    bool isActivating() const { return bIsActivating; }

    bool isActive() const;

    /// activate is called when the actor awoken
    /// activate the actor again after it's been deactivate
    void activate();

    void deactivate();

    void destroy();

    void willDestroyActor();

    template<typename T>
        requires std::is_base_of_v<Component, T>
    T *addComponent() {
        return addComponentInternal(T::GetClassIDStatic())->template asUnsafe<T>();
    }

    template<typename T>
        requires std::is_base_of_v<Component, T>
    T *getComponent() {
        if (T::IsSealedClass()) {
            return getComponentExactClassInternal(T::GetClassIDStatic())->template asUnsafe<T>();
        }
        return getComponentInternal(T::GetClassIDStatic())->template asUnsafe<T>();
    }

    Transform *getTransform() {
        return getComponent<Transform>();
    }

    Component *addComponentInternal(int id);

    Component *getComponentExactClassInternal(int id);

    Component *getComponentInternal(int id);

    ComponentContainer &getComponents() { return components; }
};

inline Transform *Component::getTransform() const {
    if (_actor == nullptr) {
        return nullptr;
    }
    return _actor->getComponent<Transform>();
}

template<typename T>
T *Component::getComponent() {
    if (_actor == nullptr) return nullptr;
    return _actor->getComponent<T>();
}

template<typename T>
T *Component::addComponent() {
    if (_actor == nullptr) return nullptr;
    return _actor->addComponent<T>();
}

class AN_API ActorManager {

    ActorList _actorList;

public:

    void addActor(ActorListNode &node);

    void removeActor(ActorListNode &node);

};

AN_API ActorManager &GetActorManager();

AN_API void DestroyActor(Actor *actor);

}

#endif//OJOIE_ACTOR_HPP
