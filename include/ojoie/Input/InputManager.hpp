//
// Created by Aleudillonam on 8/5/2022.
//

#ifndef OJOIE_INPUTMANAGER_HPP
#define OJOIE_INPUTMANAGER_HPP

#include <ojoie/Core/Window.hpp>

#include <unordered_map>

namespace AN {

enum class InputModifierFlags {
    None    = 0,
    Shift   = 1 << 0,
    Control = 1 << 1,
    Alt     = 1 << 2,
    Super   = 1 << 3,
    Caps    = 1 << 4,
    NumLock = 1 << 5
};

template<>
struct enable_bitmask_operators<InputModifierFlags> : std::true_type {};

enum class InputKey {
    None = 0,
    MouseLeftButton,
    MouseMiddleButton,
    MouseRightButton,
    MouseX,
    MouseY,
    Esc,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    _0,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,

    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,

    RightShift,
    RightControl,
    RightAlt,
    RightSuper,

    Caps,
    NumLock
};

enum class InputEvent {
    None,
    Pressed,
    Released,
    Repeat,
};

class InputBindingName {
    int index;

    static std::vector<std::string> &GetNameTable();

    friend struct std::hash<AN::InputBindingName>;

public:
    InputBindingName() : index(-1) {}

    explicit InputBindingName(const char *name) {
        auto &name_table = GetNameTable();
        auto iter = std::find(name_table.begin(), name_table.end(), name);
        if (iter == name_table.end()) {
            index = (int) name_table.size();
            name_table.emplace_back(name);
        } else {
            index = (int) (iter - name_table.begin());
        }
    }

    const char *getCString() const {
        return GetNameTable()[index].c_str();
    }

    const std::string &getString() const {
        return GetNameTable()[index];
    }

    bool operator==(const InputBindingName &other) const {
        return index == other.index;
    }
};

}

template<>
struct std::hash<AN::InputBindingName> {
    std::size_t operator()(AN::InputBindingName const& name) const noexcept {
        return std::hash<decltype(name.index)>{}(name.index);
    }
};

namespace AN {

struct InputAxisBinding {
    InputBindingName name;
    Delegate<void(float/*axis delta*/)> delegate;
    float axisValue;
};

struct InputActionBinding {
    InputBindingName name;
    InputEvent event;
    Delegate<void()> delegate;
};

class InputBinding {

    std::vector<InputAxisBinding> axisBindings;
    std::vector<InputActionBinding> actionBindings;


    void _bindAxis(const char *name, const Delegate<void(float/*axis delta*/)> &delegate) {
        axisBindings.push_back({ InputBindingName{ name }, delegate });
    }

    void _bindAction(const char *name, InputEvent event, const Delegate<void()> &delegate) {
        actionBindings.push_back({ InputBindingName{ name }, event, delegate });
    }

    friend class InputManager;
public:

    template<typename Func>
    void bindAxis(const char *name, Func &&func) {
        Delegate<void(float/*axis delta*/)> delegate;
        delegate.bind(std::forward<Func>(func));
        _bindAxis(name, delegate);
    }

    template<typename _Class, typename Method>
    void bindAxis(const char *name, _Class *self, Method &&method) {
        Delegate<void(float/*axis delta*/)> delegate;
        delegate.bind(self, std::forward<Method>(method));
        _bindAxis(name, delegate);
    }

    template<typename Func>
    void bindAction(const char *name, InputEvent action, Func &&func) {
        Delegate<void()> delegate;
        delegate.bind(std::forward<Func>(func));
        _bindAction(name, action, delegate);
    }

    template<typename _Class, typename Method>
    void bindAction(const char *name, InputEvent action, _Class *self, Method &&method) {
        Delegate<void()> delegate;
        delegate.bind(self, std::forward<Method>(method));
        _bindAction(name, action, delegate);
    }
};

class InputManager : public NonCopyable {

    struct InputActionMapping {
        InputBindingName name;
        InputKey key;
        InputModifierFlags flags;
    };

    struct InputAxisMapping {
        InputBindingName name;
        InputKey key;
        float scale;
    };

    struct KeyEvent {
        AN::InputKey key;
        AN::InputEvent event;
        AN::InputModifierFlags flags;
    };

    Window *currentWindow{};

    std::vector<InputActionMapping> actionMappings;
    std::vector<InputAxisMapping> axisMappings;

    std::unordered_map<InputBindingName, float> axisCaches;
    std::unordered_map<InputBindingName, std::vector<InputEvent>> actionCaches;


    std::unordered_map<InputKey, std::vector<KeyEvent>> keyInputs;
    std::unordered_map<InputKey, KeyEvent> keyStates;
    std::atomic<float> mousePositionX, mousePositionY;


    struct Impl;
    Impl *impl;

    InputManager();

    ~InputManager();

public:

    static InputManager &GetSharedManager();


    /// \MainThread
    void setCurrentWindow(Window *window);

    /// \MainThread
    Window *getCurrentWindow() const {
        return currentWindow;
    }


    void addActionMapping(const char *name, InputKey key, InputModifierFlags flags = InputModifierFlags::None) {
        actionMappings.push_back({ InputBindingName{ name }, key, flags });
    }

    void addAxisMapping(const char *name, InputKey key, float scale = 1.f) {
        axisMappings.push_back({ InputBindingName{ name }, key, scale });
    }


    void removeActionMapping(const char *name) {
        auto iter = std::find_if(actionMappings.begin(), actionMappings.end(), [&](const InputActionMapping &mapping) {
            return mapping.name == InputBindingName{ name };
        });
        if (iter != actionMappings.end()) {
            actionMappings.erase(iter);
        }
    }

    void removeAxisMapping(const char *name) {
        auto iter = std::find_if(axisMappings.begin(), axisMappings.end(), [&](const InputAxisMapping &mapping) {
            return mapping.name == InputBindingName{ name };
        });
        if (iter != axisMappings.end()) {
            axisMappings.erase(iter);
        }
    }

    /// \brief called by Game instance per frame
    void process(float deltaTime);

    void processInput(InputBinding &inputBinding);

    float getMousePositionX() const { return mousePositionX.load(std::memory_order_relaxed); }

    float getMousePositionY() const { return mousePositionY.load(std::memory_order_relaxed); }

    bool isKeyPress(InputKey key, InputModifierFlags flags = InputModifierFlags::None) {
        const auto &keyState = keyStates[key];
        if (keyState.event == InputEvent::Pressed || keyState.event == InputEvent::Repeat) {
            if (keyState.flags == flags) {
                return true;
            }
        }
        return false;
    }
};


inline InputManager &GetInputManager() {
    return InputManager::GetSharedManager();
}


}





#endif//OJOIE_INPUTMANAGER_HPP
