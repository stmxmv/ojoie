//
// Created by Aleudillonam on 8/6/2022.
//

#include "Input/InputManager.hpp"


#if AN_WIN
#include "Input/win32/InputManager.hpp"
#endif

namespace AN {

bool InputManager::init() {
    _keyboard.finishSetup();
    _mouse.finishSetup();
    return true;
}

void InputManager::deinit() {}

InputManager &InputManager::GetSharedManager() {
#if AN_WIN
    static WIN::InputManager manager;
#endif
    return manager;
}

void InputManager::onNextUpdate() {
    _mouse.onNextUpdate();
    _keyboard.onNextUpdate();
}

void InputManager::resetState() {
    _mouse.resetState(_mouse.getCurrentFrameStatePtr());
    _keyboard.resetState(_keyboard.getCurrentFrameStatePtr());
}

IInputControl *InputManager::tryGetControl(int path) {
    if (path >= kPointerX && path <= kMouseRightButton) {
        return getMouse().getInputControl(path);
    } else {
        return getKeyboard().getInputControl(path);
    }
}

void InputManager::update() {
    _mouse.onUpdate();
    _keyboard.onUpdate();
    for (auto &node : _inputComponentList) {
        InputComponent &inputComponent = *node;
        inputComponent.update();
    }
}


}