//
// Created by aojoie on 5/14/2023.
//

#ifndef OJOIE_INPUTBINDING_HPP
#define OJOIE_INPUTBINDING_HPP

#include <ojoie/Input/InputControl.hpp>
#include <ojoie/Configuration/typedef.h>
#include <ojoie/Math/Math.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <ranges>

namespace AN {


class IInputTrigger {
public:
    virtual ~IInputTrigger() = default;
    virtual bool isTriggered(IInputControl *control) = 0;
};

class IInputModifier {
public:
    virtual ~IInputModifier() = default;
    virtual void modify(InputControlType type, void *data) = 0;
};

class AN_API PressedTrigger : public IInputTrigger {
public:
    virtual bool isTriggered(IInputControl *control) override {
        ButtonControl *buttonControl = dynamic_cast<ButtonControl *>(control);
        if (buttonControl == nullptr) return false;
        return buttonControl->wasPressedThisFrame();
    }
};

class AN_API ReleaseTrigger : public IInputTrigger {
public:
    virtual bool isTriggered(IInputControl *control) override {
        ButtonControl *buttonControl = dynamic_cast<ButtonControl *>(control);
        if (buttonControl == nullptr) return false;
        return buttonControl->wasReleasedThisFrame();
    }
};

/// a trigger that only trigger when all modifiers kes are pressed, and the inputBinding is first press
class AN_API ModifierTrigger : public IInputTrigger {
    std::vector<int> _paths;
public:

    explicit ModifierTrigger(int path) : _paths(1, path) {}
    explicit ModifierTrigger(std::vector<int> paths) : _paths(std::move(paths)) {}

    virtual bool isTriggered(IInputControl *control) override;

};

class AN_API InputBinding {
    typedef InputBinding Self;

    int _path;
    std::vector<std::unique_ptr<IInputTrigger>> _triggers;
    std::vector<std::unique_ptr<IInputModifier>> _modifiers;

public:

    explicit InputBinding(int path) : _path(path) {}

    InputBinding(const InputBinding&) = delete;
    InputBinding &operator = (const InputBinding &) = delete;

    InputBinding(InputBinding &&) = default;

    int getPath() const { return _path; }

    /// add and transfer ownership
    Self& addTrigger(IInputTrigger *trigger) { _triggers.emplace_back(trigger); return *this; }

    Self& addModifier(IInputModifier *modifier) { _modifiers.emplace_back(modifier); return *this; }

    bool hasTrigger() { return !_triggers.empty(); }

    template<typename T, typename ..._Args>
    void addTrigger(_Args &&...args) { _triggers.push_back(std::make_unique<T>(std::forward<_Args>(args)...)); }

    template<typename T, typename ..._Args>
    void addModifier(_Args &&...args) { _modifiers.push_back(std::make_unique<T>(std::forward<_Args>(args)...)); }

    bool isTrigger(IInputControl *control) {
        if (_triggers.empty()) return true;
        return std::ranges::any_of(_triggers, [control](auto &&trigger) {
            return  trigger->isTriggered(control);
        });
    }

    void modify(InputControlType type, void *data) {
        for (auto &modifier : _modifiers) {
            modifier->modify(type, data);
        }
    }

};


class AN_API Vector2CompositeBinding {
    InputBinding up, down, left, right;

public:
    Vector2CompositeBinding(int up, int down, int left, int right)
        : up(up),
          down(down),
          left(left),
          right(right) {}

    Vector2CompositeBinding(Vector2CompositeBinding &&) = default;
    Vector2CompositeBinding(const InputBinding&) = delete;
    Vector2CompositeBinding &operator = (const Vector2CompositeBinding &) = delete;

    InputBinding &getUp() { return up; }
    InputBinding &getDown() { return down; }
    InputBinding &getLeft() { return left; }
    InputBinding &getRight() { return right; }
};


}

#endif//OJOIE_INPUTBINDING_HPP
