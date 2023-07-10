//
// Created by aojoie on 5/15/2023.
//

#include "Input/InputBinding.hpp"
#include "Input/InputManager.hpp"

namespace AN {

bool ModifierTrigger::isTriggered(IInputControl *control) {
    ButtonControl *buttonControl = dynamic_cast<ButtonControl *>(control);
    if (buttonControl == nullptr) return false;

    if (!buttonControl->wasPressedThisFrame()) {
        return false;
    }

    for (int path : _paths) {
        IInputControl *modifierControl = GetInputManager().tryGetControl(path);
        if (!modifierControl) {
            return false;
        }
        ButtonControl *modifierButtonControl = dynamic_cast<ButtonControl *>(modifierControl);

        if (modifierButtonControl == nullptr || !modifierButtonControl->isPress())
            return false;
    }
    return true;
}

}