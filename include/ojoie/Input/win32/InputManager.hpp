//
// Created by aojoie on 7/3/2023.
//

#pragma once

#include <ojoie/Input/InputManager.hpp>

#include <Windows.h>

namespace AN::WIN {

class InputManager : public AN::InputManager {

    HWND hWnd;
public:

    virtual bool init() override;

    virtual void deinit() override;
};

}
