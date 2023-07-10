#if defined(VLD_MEM_CHECK)
#include <vld.h>
#endif
#include <ojoie/BaseClasses/Object.hpp>
#include <ojoie/BaseClasses/ObjectPtr.hpp>
#include <ojoie/Math/Transform.hpp>
#include <iostream>
#include <vector>

using std::cin, std::cout, std::endl;

class Behavior : public AN::Object {

    REGISTER_DERIVED_ABSTRACT_AN_CLASS(Behavior, AN::Object);

    explicit Behavior(AN::ObjectCreationMode mode) : Super(mode) {}

};

IMPLEMENT_AN_CLASS(Behavior);

Behavior::~Behavior() {

}

class Component : public AN::Object {

    REGISTER_DERIVED_ABSTRACT_AN_CLASS(Component, AN::Object);

    explicit Component(AN::ObjectCreationMode mode) : Super(mode) {}

};

IMPLEMENT_AN_CLASS(Component);

Component::~Component() {

}

using namespace AN;

class Transform : public Component {

    REGISTER_DERIVED_AN_CLASS(Transform, Component);

    explicit Transform(AN::ObjectCreationMode mode) : Super(mode) {
        cout << "Transform ctor" << endl;
        transform.translation = { 1, 13, 1 };
    }

    Math::Transform transform;

};

IMPLEMENT_AN_CLASS(Transform);

Transform::~Transform() {
    cout << "Transform dtor" << endl;
}

void printClass(AN::Class *cls) {
    cout << cls->debugDescription() << endl;
}

class Actor {

    typedef std::vector<ObjectPtr<Component>> ComponentContainer;
    ComponentContainer  components;
    void *impl;

public:

    Actor() : impl() {}


    //    template<typename T>
    //    T *addComponent() {
    //        return (T *) addComponent(Ubpa::TypeID_of<T>.GetValue());
    //    }
    //
    //    template<typename T>
    //    T *getComponent() {
    //        return (T *) getComponent(Ubpa::TypeID_of<T>.GetValue());
    //    }
};



int main() {

    using namespace AN;
    AN::Object::RegisterClass();
    Component::RegisterClass();
    Behavior::RegisterClass();
    Transform::RegisterClass();

    Class *objectClass = Class::GetClass(Object::GetClassIDStatic());
    Class *componentClass = Class::GetClass(Component::GetClassIDStatic());
    Class *transformClass = Class::GetClass(Transform::GetClassIDStatic());


    ANAssert(objectClass->isDerivedFrom<Object>());
    ANAssert(componentClass->isDerivedFrom<Object>());
    ANAssert(transformClass->isDerivedFrom<Component>() && transformClass->isDerivedFrom<Object>());

    ANAssert(!componentClass->isDerivedFrom<Component>() && !componentClass->isDerivedFrom<Transform>());
    ANAssert(!objectClass->isDerivedFrom<Transform>());

    printClass(objectClass);
    printClass(componentClass);
    printClass(transformClass);

    Transform *transform = NewObject<Transform>();

    printClass(transform->getClass());
    printClass(transform->getSuperClass());

    DestroyObject(transform);

    transform = (Transform *)NewObject("Transform");

    printClass(transform->getClass());
    printClass(transform->getSuperClass());

    cout << transform->transform.translation.y << endl;

    Component *transComp = transform->as<Component>();
    ANAssert(transform != nullptr);
    Behavior *behavior = transform->as<Behavior>();
    ANAssert(behavior == nullptr);

    DestroyObject(transform);

    Component *component = NewObject<Component>();

    ANAssert(component == nullptr);

    Actor actor;


    return 0;
}
