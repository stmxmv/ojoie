//
// Created by aojoie on 5/15/2023.
//

#ifndef OJOIE_INPUTDEVICE_HPP
#define OJOIE_INPUTDEVICE_HPP

#include <ojoie/Input/InputControl.hpp>
#include <ojoie/Input/InputControlPath.hpp>
#include <ojoie/Math/Math.hpp>

#include <bitset>
#include <queue>

namespace AN {

class IInputDevice {
public:

    virtual ~IInputDevice() = default;

    virtual void onNextUpdate() {}
    virtual void onUpdate() {}
    virtual void finishSetup() {}
    virtual void *getCurrentFrameStatePtr() = 0;
    virtual void *getPreviousFrameStatePtr() = 0;

    virtual IInputControl *getInputControl(int path) = 0;

    virtual void resetState(void *statePtr) = 0;

    void resetDeviceState() {
        resetState(getCurrentFrameStatePtr());
        resetState(getPreviousFrameStatePtr());
    }

};

template<typename State>
class InputDevice : public IInputDevice {
protected:
    State _currentFrameState;
    State _previousFrameState;

public:

    InputDevice() {
        _currentFrameState.reset();
        _previousFrameState.reset();
    }

    virtual void onNextUpdate() override {
        _previousFrameState = _currentFrameState;
    }

    void setCurrentStateInternal(const State &state) { _currentFrameState = state; }
    State &getCurrentFrameState() { return _currentFrameState; }
    State &getPreviousFrameState() { return _previousFrameState; }

    virtual void *getCurrentFrameStatePtr() override {
        return &_currentFrameState;
    }
    virtual void *getPreviousFrameStatePtr() override {
        return &_previousFrameState;
    }

    virtual void resetState(void *statePtr) override {
        State *state = (State *)statePtr;
        state->reset();
    }
};

struct MouseState {
    Vector2f mouseDelta;
    Vector2f mousePos;
    Vector2f mouseScroll;
    std::bitset<3> mouseButtons;

    void reset() {
        memset(this, 0, sizeof *this);
    }
};

class AN_API Mouse : public InputDevice<MouseState> {

    Vector2Control _mouseDelta;
    Vector2Control _mousePos;
    Vector2Control _mouseScroll;
    ButtonControl  _mouseLeft;
    ButtonControl  _mouseMiddle;
    ButtonControl  _mouseRight;

    ANQueue<std::pair<int, bool>> _buttonStateQueue;

public:

    Mouse() {
        _mouseDelta.initInternal(this, kInputStateFormatVector2, 0, 0);
        _mousePos.initInternal(this, kInputStateFormatVector2, offsetof(MouseState, mousePos), 0);
        _mouseScroll.initInternal(this, kInputStateFormatVector2, offsetof(MouseState, mouseScroll), 0);
        _mouseLeft.initInternal(this, kInputStateFormatBit, offsetof(MouseState, mouseButtons), 0);
        _mouseMiddle.initInternal(this, kInputStateFormatBit, offsetof(MouseState, mouseButtons), 1);
        _mouseRight.initInternal(this, kInputStateFormatBit, offsetof(MouseState, mouseButtons), 2);
    }



    virtual void onNextUpdate() override {
        InputDevice::onNextUpdate();
        _currentFrameState.mouseScroll = {};
        _currentFrameState.mouseDelta = {};
    }

    virtual void finishSetup() override {
        _mouseDelta.finishSetup();
        _mousePos.finishSetup();
        _mouseScroll.finishSetup();
        _mouseLeft.finishSetup();
        _mouseMiddle.finishSetup();
        _mouseRight.finishSetup();
    }

    Vector2Control &getMouseDelta() { return _mouseDelta; }
    Vector2Control &getMousePos() { return _mousePos; }
    Vector2Control &getMouseScroll() { return _mouseScroll; }
    ButtonControl &getMouseLeft() { return _mouseLeft; }
    ButtonControl &getMouseMiddle() { return _mouseMiddle; }
    ButtonControl &getMouseRight() { return _mouseRight; }

    void queueDelta(const Vector2f &delta, const Vector2f &scroll) {
        getCurrentFrameState().mouseDelta += delta;
        getCurrentFrameState().mouseScroll += scroll;
    }

    void queueButtonState(int button, bool state) {
        _buttonStateQueue.emplace(button ,state);
    }

    virtual void onUpdate() override {
        ANArray<std::pair<int, bool>> remainButtonState;
        std::bitset<3> buttonStateChange{};

        while (!_buttonStateQueue.empty()) {
            std::pair<int, bool> state = _buttonStateQueue.front();
            _buttonStateQueue.pop();

            if (buttonStateChange[state.first]) {
                /// process the state in next frame
                remainButtonState.push_back(state);
            } else {
                buttonStateChange[state.first] = true;
                getCurrentFrameState().mouseButtons[state.first] = state.second;
            }
        }

        if (!remainButtonState.empty()) {
            _buttonStateQueue.push_range(remainButtonState);
        }
    }

    virtual IInputControl *getInputControl(int path) override {
        switch (path) {
            case kPointerX:
                return &_mouseDelta.getX();
            case kPointerY:
                return &_mouseDelta.getY();
            case kPointerDelta:
                return &_mouseDelta;
            case kMouseScroll:
                return &_mouseScroll;
            case kMouseLeftButton:
                return &_mouseLeft;
            case kMouseMiddleButton:
                return &_mouseMiddle;
            case kMouseRightButton:
                return &_mouseRight;
            default:
                return nullptr;
        }
        return nullptr;
    }
};

struct KeyboardState {
    typedef std::bitset<kInputControlPathMaxNum> KeyBitSet;
    static_assert(sizeof(KeyBitSet) == kInputControlPathMaxNum / 8);

    KeyBitSet keyStates;

    void reset() {
        keyStates.reset();
    }
};

class AN_API Keyboard : public InputDevice<KeyboardState> {
    ButtonControl _buttonControls[kInputControlPathMaxNum];

    ANQueue<std::pair<int, bool>> _buttonStateQueue;

    bool isKeySupport(int key) {
        return (key >= kInputKeyLeftShift && key <= kInputKeyKeypadEnter) ||
               (key >= kInputKey_0 && key <= kInputKey_9) ||
               (key >= kInputKey_A && key <= kInputKey_Z) ||
               (key >= kInputKey_F1 && key <= kInputKey_F12) ||
               key == kInputKeyTab || key == kInputKeyEsc || key == kInputKeyEnter || key == kInputKeySpace;
    }

public:

    Keyboard() {
        for (int i = kInputKeyLeftShift; i <= kInputKeyKeypadEnter; ++i) {
            _buttonControls[i].initInternal(this, kInputStateFormatBit, i / 8, i % 8);
        }
        for (int i = kInputKey_0; i <= kInputKey_9; ++i) {
            _buttonControls[i].initInternal(this, kInputStateFormatBit, i / 8, i % 8);
        }
        for (int i = kInputKey_A; i <= kInputKey_Z; ++i) {
            _buttonControls[i].initInternal(this, kInputStateFormatBit, i / 8, i % 8);
        }
        for (int i = kInputKey_F1; i <= kInputKey_F12; ++i) {
            _buttonControls[i].initInternal(this, kInputStateFormatBit, i / 8, i % 8);
        }
        _buttonControls[kInputKeyEsc].initInternal(this, kInputStateFormatBit, kInputKeyEsc / 8, kInputKeyEsc % 8);
        _buttonControls[kInputKeyEnter].initInternal(this, kInputStateFormatBit, kInputKeyEnter / 8, kInputKeyEnter % 8);
        _buttonControls[kInputKeySpace].initInternal(this, kInputStateFormatBit, kInputKeySpace / 8, kInputKeySpace % 8);
        _buttonControls[kInputKeyTab].initInternal(this, kInputStateFormatBit, kInputKeyTab / 8, kInputKeyTab % 8);
    }

    virtual void finishSetup() override {
        for (int i = kInputKeyLeftShift; i <= kInputKeyKeypadEnter; ++i) {
            _buttonControls[i].finishSetup();
        }
        for (int i = kInputKey_0; i <= kInputKey_9; ++i) {
            _buttonControls[i].finishSetup();
        }
        for (int i = kInputKey_A; i <= kInputKey_Z; ++i) {
            _buttonControls[i].finishSetup();
        }
        for (int i = kInputKey_F1; i <= kInputKey_F12; ++i) {
            _buttonControls[i].finishSetup();
        }
        _buttonControls[kInputKeyEsc].finishSetup();
        _buttonControls[kInputKeyEnter].finishSetup();
        _buttonControls[kInputKeySpace].finishSetup();
        _buttonControls[kInputKeyTab].finishSetup();
    }

    virtual void onUpdate() override {
        ANArray<std::pair<int, bool>> remainButtonState;
        KeyboardState::KeyBitSet buttonStateChange{};

        while (!_buttonStateQueue.empty()) {
            std::pair<int, bool> state = _buttonStateQueue.front();
            _buttonStateQueue.pop();

            if (buttonStateChange[state.first]) {
                /// process the state in next frame
                remainButtonState.push_back(state);
            } else {
                buttonStateChange[state.first] = true;
                getCurrentFrameState().keyStates[state.first] = state.second;
            }
        }

        if (!remainButtonState.empty()) {
            _buttonStateQueue.push_range(remainButtonState);
        }
    }

    void queueButtonState(int button, bool state) {
        _buttonStateQueue.emplace(button ,state);
    }

    ButtonControl *getButtonControl(int key) {
        if (!isKeySupport(key)) return nullptr;
        return &_buttonControls[key];
    }

    virtual IInputControl *getInputControl(int path) override {
        if (!isKeySupport(path)) return nullptr;
        return &_buttonControls[path];
    }
};


}

#endif//OJOIE_INPUTDEVICE_HPP
