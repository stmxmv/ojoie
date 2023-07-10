//
// Created by aojoie on 5/15/2023.
//

#include "Input/InputManager.hpp"
#include "Input/InputAction.hpp"

namespace AN {

IMPLEMENT_AN_CLASS(InputActionMap)
LOAD_AN_CLASS(InputActionMap)

InputActionMap::~InputActionMap() {}

void InputAction::process(InputActionCallback callback, void *userdata) {

    for (Vector2CompositeBinding &binding : _vector2CompositeBindings) {
        int up = binding.getUp().getPath();
        int down = binding.getDown().getPath();
        int left = binding.getLeft().getPath();
        int right = binding.getRight().getPath();

        Vector2f value{};
        AxisControl *upControl = GetInputManager().tryGetControl<AxisControl>(up);
        AxisControl *downControl = GetInputManager().tryGetControl<AxisControl>(down);
        AxisControl *leftControl = GetInputManager().tryGetControl<AxisControl>(left);
        AxisControl *rightControl = GetInputManager().tryGetControl<AxisControl>(right);

        if (upControl && downControl && leftControl && rightControl) {
            if (binding.getUp().isTrigger(upControl) &&
                binding.getDown().isTrigger(downControl) &&
                binding.getLeft().isTrigger(leftControl) &&
                binding.getRight().isTrigger(rightControl)) {

                float axisValue = upControl->getValue();
                binding.getUp().modify(kInputControlAxis, &value);

                value.y += axisValue;

                axisValue = downControl->getValue();
                binding.getDown().modify(kInputControlAxis, &value);

                value.y -= axisValue;

                axisValue = leftControl->getValue();
                binding.getLeft().modify(kInputControlAxis, &value);

                value.x -= axisValue;

                axisValue = rightControl->getValue();
                binding.getRight().modify(kInputControlAxis, &value);

                value.x += axisValue;

                value = Math::normalize(value);

                callback(_name, &value, kInputControlVector2, userdata);
                return;
            }
        }
    }

    for (InputBinding &binding : _inputBindings) {
        int path = binding.getPath();
        IInputControl *control = GetInputManager().tryGetControl(path);
        if (control == nullptr) continue;

        switch (_controlType) {
            case kInputControlButton:
            {
                ButtonControl *buttonControl = dynamic_cast<ButtonControl *>(control);
                if (!buttonControl) continue;
                if (binding.hasTrigger()) {
                    if (binding.isTrigger(buttonControl)) {
                        /// button don't have value
                        callback(_name, nullptr, kInputControlButton, userdata);
                        return;
                    }
                } else if (buttonControl->wasPressedThisFrame()) {
                    /// button don't have value
                    callback(_name, nullptr, kInputControlButton, userdata);
                    return;
                }
            }
                break;
            case kInputControlAxis:
            {
                AxisControl *axisControl = dynamic_cast<AxisControl *>(control);
                if (!axisControl) continue;
                if (binding.isTrigger(axisControl)) {
                    float value = axisControl->getValue();
                    binding.modify(kInputControlAxis, &value);
                    callback(_name, &value, kInputControlAxis, userdata);
                    return;
                }
            }
                break;
            case kInputControlVector2:
            {
                Vector2Control *vector2Control = dynamic_cast<Vector2Control *>(control);
                if (!vector2Control) continue;
                if (binding.isTrigger(vector2Control)) {
                    Vector2f vector2 = vector2Control->getValue();
                    binding.modify(kInputControlVector2, &vector2);
                    callback(_name, &vector2, kInputControlVector2, userdata);
                    return;
                }
            }
                break;
        }
    }
}

void InputActionMap::process(InputActionCallback callback, void *userdata) {
    for (InputAction &inputAction : _inputActions) {
        inputAction.process(callback, userdata);
    }
}


}