//
// Created by aojoie on 3/31/2023.
//

#ifndef OJOIE_COMPONENT_HPP
#define OJOIE_COMPONENT_HPP

#include <ojoie/Object/Object.hpp>

namespace AN {

class Transform;

class AN_API Component : public Object {

    class Actor *_actor;

    bool m_IsDestroying;

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Component, Object);

    explicit Component(ObjectCreationMode mode);

    virtual void dealloc() override;

    // Returns a reference to the GameObject holding this component
    Actor& getActor()					{ return *_actor; }
    const Actor& getActor () const		{ return *_actor; }
    Actor* getActorPtr ()		{ return _actor; }
    Actor* getActorPtr () const	{ return _actor; }

    /// activate the component
    virtual void activate() {}

    virtual void deactivate() {}

    /// default depend on actor's active state
    virtual bool isActive() const;

    virtual void willDestroyComponent() {}

    inline Transform *getTransform() const;

    template<typename T>
    T *getComponent();

    template<typename T>
    T *addComponent();

    /// SetActor is called whenever the Actor of a component changes.
    void setActorInternal(Actor* actor) { _actor = actor; }

    virtual void onInspectorGUI() {}

    friend class Actor;
};

}

#endif//OJOIE_COMPONENT_HPP
