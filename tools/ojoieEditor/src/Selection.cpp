//
// Created by Aleudillonam on 7/9/2023.
//

#include "Selection.hpp"

namespace AN::Editor {


static Object *s_ActiveObject;

void Selection::SetActiveObject(Object *obj) {
    s_ActiveObject = obj;
}

Object *Selection::GetActiveObject() {
    return s_ActiveObject;
}

Actor *Selection::GetActiveActor() {
    if (s_ActiveObject == nullptr) return nullptr;
    if (s_ActiveObject->getClassID() == Actor::GetClassIDStatic()) {
        return (Actor *)s_ActiveObject;
    }
    if (s_ActiveObject->getClass()->isDerivedFrom<Component>()) {
        return s_ActiveObject->as<Component>()->getActorPtr();
    }
    return nullptr;
}

}