//
// Created by Aleudillonam on 8/6/2022.
//

#include "Input/InputManager.hpp"

#include <readerwriterqueue/readerwriterqueue.hpp>

/// currently only support glfw
#include "glfw/InputManager.hpp"

struct MouseDelta {
    float deltaX;
    float deltaY;
};

struct KeyEvent {
    AN::InputKey key;
    AN::InputEvent event;
    AN::InputModifierFlags flags;
};

struct AN::InputManager::Impl {


    PlatformImpl platformImpl{};

    moodycamel::ReaderWriterQueue<MouseDelta> mouseQueue;
    moodycamel::ReaderWriterQueue<KeyEvent> keyEventQueue;

};

#include "glfw/InputManager.cpp"




namespace AN {



std::vector<std::string> &InputBindingName::GetNameTable() {
    static std::vector<std::string> name_table;
    return name_table;
}


InputManager::InputManager() : impl(new Impl()){
}

InputManager::~InputManager() {

    delete impl;
}

InputManager &InputManager::GetSharedManager() {
    static InputManager manager;
    return manager;
}



void InputManager::process(float deltaTime) {
    axisCaches.clear();
    actionCaches.clear();

    /// InputManager is a global singleton class owned by game, use static is good for private impl
    static std::unordered_map<InputKey, float> axisInputs;
    static std::unordered_map<InputKey, std::vector<KeyEvent>> keyInputs;
    keyInputs.clear();

    axisInputs[InputKey::MouseX] = 0.f;
    axisInputs[InputKey::MouseY] = 0.f;
    MouseDelta mouseDelta;
    while (impl->mouseQueue.try_dequeue(mouseDelta)) {
        axisInputs[InputKey::MouseX] += mouseDelta.deltaX;
        axisInputs[InputKey::MouseY] += mouseDelta.deltaY;
    }

    KeyEvent keyEvent;
    while (impl->keyEventQueue.try_dequeue(keyEvent)) {
        keyInputs[keyEvent.key].push_back(keyEvent);

        if (keyEvent.event == InputEvent::Pressed) {
            axisInputs[keyEvent.key] = 1.f;
        } else if (keyEvent.event == InputEvent::Released) {
            axisInputs[keyEvent.key] = 0.f;
        }
    }

    for (auto &axisMap : axisMappings) {
        if (axisInputs.contains(axisMap.key)) {
            float input = axisInputs.at(axisMap.key);
            axisCaches[axisMap.name] += input * axisMap.scale;
        }
    }

    for (auto &actionMap : actionMappings) {
        if (keyInputs.contains(actionMap.key)) {
            const auto &inputs = keyInputs.at(actionMap.key);
            for (const auto &event : inputs) {
                if (event.flags == actionMap.flags) {
                    actionCaches[actionMap.name].push_back(event.event);
                }
            }
        }
    }


}


void InputManager::processInput(InputBinding &inputBinding) {
    for (auto &axisMap : inputBinding.axisBindings) {
        if (axisCaches.contains(axisMap.name)) {
            float value = axisCaches.at(axisMap.name);
            axisMap.axisValue = value;
            axisMap.delegate(value);
        }
    }

    for (auto &actionMap : inputBinding.actionBindings) {
        if (actionCaches.contains(actionMap.name)) {
            const auto &inputEvents = actionCaches.at(actionMap.name);
            for (const auto &event : inputEvents) {
                if (event == actionMap.event) {
                    actionMap.delegate();
                }
            }
        }
    }

}


}