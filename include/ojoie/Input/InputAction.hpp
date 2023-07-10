//
// Created by aojoie on 5/15/2023.
//

#ifndef OJOIE_INPUTACTION_HPP
#define OJOIE_INPUTACTION_HPP

#include <ojoie/Input/InputControl.hpp>
#include <ojoie/Input/InputBinding.hpp>
#include <ojoie/Core/Name.hpp>
#include <ojoie/Object/NamedObject.hpp>
#include <vector>

namespace AN {


typedef void (*InputActionCallback)(Name action, void *value, InputControlType type, void *userdata);

class AN_API InputAction {
    Name _name;
    InputControlType _controlType;
    std::vector<InputBinding> _inputBindings;
    std::vector<Vector2CompositeBinding> _vector2CompositeBindings;
public:

    InputAction(Name name, InputControlType type) : _name(name), _controlType(type) {}
    InputAction(InputAction &&) = default;
    InputAction(const InputAction&) = delete;
    InputAction &operator = (const InputAction &) = delete;

    InputBinding &addBinding(int path) {
        _inputBindings.emplace_back(path);
        return _inputBindings.back();
    }

    Vector2CompositeBinding &addVector2CompositeBinding(int up, int down, int left, int right) {
        _vector2CompositeBindings.emplace_back(up, down, left, right);
        return _vector2CompositeBindings.back();
    }

    // will process only one triggered binding
    void process(InputActionCallback callback, void *userdata);

};

class AN_API InputActionMap : public NamedObject {

    std::vector<InputAction> _inputActions;

    DECLARE_DERIVED_AN_CLASS(InputActionMap, NamedObject)

public:

    explicit InputActionMap(ObjectCreationMode mode) : Super(mode) {}

    InputActionMap(InputAction &&) = delete;
    InputActionMap(const InputActionMap&) = delete;
    InputActionMap &operator = (const InputActionMap &) = delete;

    InputAction &addAction(Name name, InputControlType type) {
        _inputActions.emplace_back(name, type);
        return _inputActions.back();
    }

    void process(InputActionCallback callback, void *userdata);

};


}

#endif//OJOIE_INPUTACTION_HPP
