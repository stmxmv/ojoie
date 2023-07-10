//
// Created by aojoie on 4/2/2023.
//

#ifndef OJOIE_BEHAVIOR_HPP
#define OJOIE_BEHAVIOR_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Template/LinkedList.hpp>
#include <list>
#include <utility>
#include <map>

namespace AN {

class Behavior;
typedef ListNode<Behavior> BehaviourListNode;
typedef List<BehaviourListNode> BehaviorList;

class AN_API Behavior : public Component {

    BehaviourListNode _updateNode{ this };

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Behavior, Component);

private:

    bool bIsActive:1;
    bool bIsAddedToManager:1;
    bool bStartCalled:1;

    static void OnAddComponentMessage(void *receiver, Message &message);

    void updateActiveState(bool state);

public:

    /// Behavior created is active
    explicit Behavior(ObjectCreationMode mode)
        : Super(mode), bIsActive(true), bIsAddedToManager() {}

    static void InitializeClass();

    static void DeallocClass();

    virtual bool init() override;

    virtual void start() {}
    virtual void update();

    bool isAddedToManager() const { return bIsAddedToManager; }

    virtual void activate() override;

    virtual bool isActive() const override {
        return Super::isActive() && bIsActive;
    }

    virtual void deactivate() override;

    virtual void addToManager();

    virtual void removeFromManager();
};



class AN_API BehaviorManager {

    // Need to use map instead of vector_map here, because it can change during iteration
    // (Behaviours added in update calls).
    typedef std::map<int, std::pair<BehaviorList *, BehaviorList *>> Lists;

    Lists _lists;

protected:

    void integrateLists();

    template<typename T>
    void CommonUpdate();

public:

    virtual ~BehaviorManager();

    virtual void update() = 0;

    void addBehavior(BehaviourListNode &behavior, int queue);

    void removeBehavior(BehaviourListNode &behavior);

};

AN_API BehaviorManager &GetBehaviorManager();

}

#endif//OJOIE_BEHAVIOR_HPP
