//
// Created by aojoie on 4/1/2023.
//

#include <gtest/gtest.h>
#include <ojoie/BaseClasses/Object.hpp>
#include <ojoie/BaseClasses/ObjectPtr.hpp>
#include <ojoie/Math/Transform.hpp>
#include <iostream>
#include <vector>

using namespace std;

class Behavior : public AN::Object {

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Behavior, AN::Object);

    explicit Behavior(AN::ObjectCreationMode mode) : Super(mode) {}

};
//
IMPLEMENT_AN_CLASS(Behavior);
LOAD_AN_CLASS(Behavior);

Behavior::~Behavior() {

}

class Component : public AN::Object {

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Component, AN::Object);

    explicit Component(AN::ObjectCreationMode mode) : Super(mode) {}

};

IMPLEMENT_AN_CLASS(Component);
LOAD_AN_CLASS(Component);

Component::~Component() {

}

using namespace AN;

class Transform : public Component {

    DECLARE_DERIVED_AN_CLASS(Transform, Component);

    explicit Transform(AN::ObjectCreationMode mode) : Super(mode) {
        cout << "Transform ctor" << endl;
        transform.translation = { 1, 13, 1 };
    }

    static void InitializeClass() {
        cout << "initialize Transform class" << endl;
    }

    static void DeallocClass() {
        cout << "Dealloc Transform class" << endl;
    }

    Math::Transform transform;

};

IMPLEMENT_AN_CLASS(Transform);
LOAD_AN_CLASS(Transform);

Transform::~Transform() {
    cout << "Transform dtor" << endl;
}

void printClass(AN::Class *cls) {
    cout << cls->debugDescription() << endl;
}

extern "C" void __an_debugPrintClassList_internal(void);

// Demonstrate some basic assertions.
TEST(Refection, Object) {
//    AN::Object::RegisterClass();
//    Component::RegisterClass();
//    Behavior::RegisterClass();
//    Transform::RegisterClass();
    Transform::LoadClass();

    cout << "\n\n======class list\n";
    __an_debugPrintClassList_internal();
    cout << "======class list" << "\n\n";

    Class *objectClass = Class::GetClass(Object::GetClassIDStatic());
    Class *componentClass = Class::GetClass(Component::GetClassIDStatic());
    Class *transformClass = Class::GetClass(Transform::GetClassIDStatic());


    EXPECT_TRUE(objectClass->isDerivedFrom<Object>());
    EXPECT_TRUE(componentClass->isDerivedFrom<Object>());
    EXPECT_TRUE(transformClass->isDerivedFrom<Component>() && transformClass->isDerivedFrom<Object>());

    EXPECT_TRUE(!componentClass->isDerivedFrom<Component>() && !componentClass->isDerivedFrom<Transform>());
    EXPECT_TRUE(!objectClass->isDerivedFrom<Transform>());

    printClass(objectClass);
    printClass(componentClass);
    printClass(transformClass);

    Transform *transform = NewObject<Transform>();
    transform->init();
    cout << "instance id: " << transform->getInstanceID() << endl;
    ANAssert(transform == Class::GetInstance(transform->getInstanceID()));

    printClass(transform->getClass());
    printClass(transform->getSuperClass());

    DestroyObject(transform);
    ANAssert(transform == nullptr);

    transform = (Transform *)NewObject("Transform");
    transform->init();
    cout << "instance id: " << transform->getInstanceID() << endl;
    ANAssert(transform == Class::GetInstance(transform->getInstanceID()));

    printClass(transform->getClass());
    printClass(transform->getSuperClass());

    cout << transform->transform.translation.y << endl;

    Component *transComp = transform->as<Component>();
    EXPECT_TRUE(transform != nullptr);
    Behavior *behavior = transform->as<Behavior>();
    EXPECT_TRUE(behavior == nullptr);

    DestroyObject(transform);

    Component *component = NewObject<Component>();
//    transform->init();
    EXPECT_TRUE(component == nullptr);

    ObjectPtr<Component> componentPtr = MakeObjectPtr<Component>();
//    componentPtr->init();
    EXPECT_TRUE(componentPtr == nullptr);

    ObjectPtr<Transform> transformPtr = MakeObjectPtr<Transform>();
    transformPtr->init();
    EXPECT_TRUE(transformPtr);

    ObjectPtr<Transform> transformPtr1 = std::move(transformPtr);
    EXPECT_TRUE(transformPtr1 != nullptr);
    EXPECT_TRUE(transformPtr == nullptr);

    componentPtr = MakeObjectPtr<Transform>();
    componentPtr->init();
    EXPECT_TRUE(componentPtr != nullptr);

    printClass(componentPtr->getClass());

    transformPtr = dyn_cast_transfer<Transform>(componentPtr);
    EXPECT_TRUE(transformPtr != nullptr);
    EXPECT_TRUE(componentPtr == nullptr);

    cout << transformPtr->transform.translation.z << endl;

    /// dyn cast fail will not transfer ownership
    ObjectPtr<Behavior> behaviorPtr = dyn_cast_transfer<Behavior>(transformPtr);
    EXPECT_TRUE(behaviorPtr == nullptr);
    EXPECT_TRUE(transformPtr != nullptr);

    /// unsafe cast fail always transfers ownership
    behaviorPtr = unsafe_cast_transfer<Behavior>(transformPtr);
    EXPECT_TRUE(behaviorPtr != nullptr);
    EXPECT_TRUE(transformPtr == nullptr);

    printClass(behaviorPtr->getClass());

    /// move test
    transformPtr = std::move(transformPtr1);
    EXPECT_TRUE(transformPtr1 == nullptr);
    EXPECT_TRUE(transformPtr != nullptr);

    // empty pointer
    ObjectPtr<Behavior> empty = dyn_cast_transfer<Behavior>(ObjectPtr<Component>{});
    EXPECT_TRUE(empty == nullptr);

    empty = unsafe_cast_transfer<Behavior>(ObjectPtr<Component>());
    EXPECT_TRUE(empty == nullptr);
}