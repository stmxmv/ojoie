//
// Created by Aleudillonam on 8/5/2022.
//

#ifndef OJOIE_INPUTMANAGER_HPP
#define OJOIE_INPUTMANAGER_HPP

#include <ojoie/Core/Window.hpp>
#include <ojoie/Template/delegate.hpp>

#include <ojoie/Math/Math.hpp>
#include <ojoie/Input/InputControl.hpp>
#include <ojoie/Input/InputDevice.hpp>
#include <ojoie/Input/InputComponent.hpp>
#include <ojoie/Template/LinkedList.hpp>

#include <unordered_map>
#include <bitset>

namespace AN {

typedef ListNode<InputComponent> InputComponentListNode;
typedef List<InputComponentListNode> InputComponentList;

class IInputControl;

class AN_API InputManager : public NonCopyable {

    Mouse    _mouse;
    Keyboard _keyboard;

    InputComponentList _inputComponentList;

public:

    static InputManager &GetSharedManager();

    virtual bool init();

    virtual void deinit();

    void onNextUpdate();

    void resetState();

    Mouse &getMouse() { return _mouse; }
    Keyboard &getKeyboard() { return _keyboard; }

    IInputControl *tryGetControl(int path);

    IInputControl *getControl(int path) {
        IInputControl *control = tryGetControl(path);
        if (control == nullptr) {
            throw AN::Exception("invalid control path");
        }
        return control;
    }

    template<typename T>
    T *tryGetControl(int path) {
        IInputControl *control = tryGetControl(path);
        T *casted = dynamic_cast<T *>(control);
        return casted;
    }

    template<typename T>
    T *getControl(int path) {
        T *control = tryGetControl<T>(path);
        if (control == nullptr) {
            throw AN::Exception("invalid control path");
        }
        return control;
    }


    void addInputComponent(InputComponentListNode &node) {
        _inputComponentList.push_back(node);
    }

    void removeInputComponent(InputComponentListNode &node) {
        node.removeFromList();
    }

    /// \brief called by Game instance per frame
    void update();

};


inline InputManager &GetInputManager() {
    return InputManager::GetSharedManager();
}


}





#endif//OJOIE_INPUTMANAGER_HPP
