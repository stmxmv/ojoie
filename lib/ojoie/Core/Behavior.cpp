//
// Created by aojoie on 4/2/2023.
//

#include "Core/Behavior.hpp"
#include "Core/Actor.hpp"
#include "Template/Iterator.hpp"

namespace AN {

IMPLEMENT_AN_CLASS_INIT_DLC(Behavior);
LOAD_AN_CLASS(Behavior);

Behavior::~Behavior() {

}

void Behavior::updateActiveState(bool state) {
    bool shouldAdded = state && bIsActive;
    if (shouldAdded == bIsAddedToManager) {
        return;
    }

    if (shouldAdded) {
        bIsAddedToManager = true;
        addToManager();
    } else {
        bIsAddedToManager = false;
        removeFromManager();
    }
}

void Behavior::activate() {
    Super::activate();
    updateActiveState(true);
}

void Behavior::deactivate() {
    updateActiveState(false);
    Super::deactivate();
}

bool Behavior::init() {
    updateActiveState(isActive());
    bStartCalled = false;
    return Super::init();
}

void Behavior::update() {
    if (!bStartCalled) {
        start();
        bStartCalled = true;
    }
}

BehaviorManager::~BehaviorManager() {
    for (Lists::iterator i = _lists.begin(); i != _lists.end(); i++) {
        Lists::mapped_type &listPair = i->second;

        ANAssert(listPair.first == NULL || listPair.first->empty());
        delete listPair.first;

        ANAssert(listPair.second == NULL || listPair.second->empty());
        delete listPair.second;
    }
    _lists.clear();
}

void BehaviorManager::update() {}

void Behavior::addToManager() {
    GetBehaviorManager().addBehavior(_updateNode, 0);
}

void Behavior::removeFromManager() {
    GetBehaviorManager().removeBehavior(_updateNode);
}

void BehaviorManager::addBehavior(BehaviourListNode &behavior, int queue) {
    Lists::mapped_type& listPair = _lists[queue];
    if (listPair.first == nullptr) {
        ANAssert(listPair.second == nullptr);
        listPair.first = new BehaviorList();
        listPair.second = new BehaviorList();
    }

    listPair.second->push_back(behavior);
}

void BehaviorManager::removeBehavior(BehaviourListNode &behavior) {
    behavior.removeFromList();
}

void BehaviorManager::integrateLists() {
    for (Lists::iterator i = _lists.begin(); i != _lists.end(); ++i) {
        Lists::mapped_type& listPair = (*i).second;

        listPair.first->append(*listPair.second);
        ANAssert(listPair.second->empty());
    }
}

template<typename T>
void BehaviorManager::CommonUpdate() {
    T &self = static_cast<T &>(*this);

    integrateLists();

    for (Lists::iterator i = _lists.begin(); i != _lists.end(); ++i) {
        Lists::mapped_type& listPair = (*i).second;

        SafeIterator<BehaviorList> iterator (*listPair.first);
        while (iterator.next()) {
            Behavior& behaviour = **iterator;

#if SUPPORT_LOG_ORDER_TRACE
            if (RunningReproduction())
            {
                if (SUPPORT_LOG_ORDER_TRACE == 2)
                {
                    LogString(Format("UpdateBehaviour %s (%s) [%d]", behaviour.GetName(), behaviour.GetClassName().c_str(), behaviour.GetInstanceID()));
                }
                else
                {
                    LogString(Format("UpdateBehaviour %s (%s)", behaviour.GetName(), behaviour.GetClassName().c_str()));
                }
            }
#endif

            ANAssert(behaviour.isAddedToManager ());

            self.updateBehavior(behaviour);

// Behaviour might get destroyed in the mean time, so we have to check if the object still exists first
#ifdef AN_DEBUG
            ANAssert(behaviour.isActive() && behaviour.isAddedToManager());
#endif
        }
    }
}

class UpdateManager : public BehaviorManager {

public:

    void updateBehavior(Behavior &behavior) {
        behavior.update();
    }

    virtual void update() override {
        BehaviorManager::CommonUpdate<UpdateManager>();
    }
};

static UpdateManager *gUpdateManager;


void Behavior::OnAddComponentMessage(void *receiver, Message &message) {
    Behavior *self = (Behavior *)receiver;
    if (message.getData<Behavior *>() == self) {
        self->updateActiveState(self->isActive());
    }
}

void Behavior::InitializeClass() {
    gUpdateManager = new UpdateManager();
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage, OnAddComponentMessage);
}

void Behavior::DeallocClass() {
    delete gUpdateManager;
}

BehaviorManager &GetBehaviorManager() {
    return reinterpret_cast<BehaviorManager &>(*gUpdateManager);
}

}