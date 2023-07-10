//
// Created by aojoie on 5/15/2023.
//

#include "Input/InputControl.hpp"
#include "Input/InputDevice.hpp"

namespace AN {


void *IInputControl::getCurrentFrameStatePtr() {
    return _device->getCurrentFrameStatePtr();
}

void *IInputControl::getPreviousFrameStatePtr() {
    return _device->getPreviousFrameStatePtr();
}

}