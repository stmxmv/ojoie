//
// Created by aojoie on 5/15/2023.
//

#include "Input/InputComponent.hpp"
#include "Input/InputManager.hpp"
#include "Core/Actor.hpp"

#include <format>

namespace AN {

IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(InputComponent)
LOAD_AN_CLASS(InputComponent)

InputComponent::~InputComponent() {}

void InputComponent::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage,
                                              [](void *receiver, Message &message) {
                                                  InputComponent *self = (InputComponent *) receiver;
                                                  if (message.getData<Component *>() == self) {
                                                      /// only do when the added component is self
                                                      GetInputManager().addInputComponent(self->_listNode);
                                                  }
                                              });
}

bool InputComponent::init() {
    if (!Super::init()) return false;
    return true;
}

void InputComponent::update() {
    if (_actionMap == nullptr) return;
    _actionMap->process([](Name action, void *value, InputControlType type, void *userdata) {
        InputComponent *self = (InputComponent *)userdata;

        Message message;
        message.name = std::format("On{}", action.string_view());
        message.data = (intptr_t)value;
        message.sender = self;

        self->getActor().sendMessage(message);

    }, this);
}

}