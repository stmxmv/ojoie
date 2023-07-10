//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_DELEGATE_HPP
#define OJOIE_DELEGATE_HPP

#include <ojoie/Core/Exception.hpp>

namespace AN {

/// \brief a delegate is like a fast callback,
///        support both raw function pointer, class pointer and functors whose size <= 24 and has trivial destructor
///
/// \attention don't forget to call unbind() if the delegate's life time is less than the delegator.
///
/// \example
///     assume AClass has member Delegate<void()> didSomething;
///
///     AClass aClass;
///
///     aClass.didSomething.bind(this, &SomeClass::SomeMethod); // method has sig of void()
///     aClass.didSomething = someFunction; // function has sig of void(*)()
///     aClass.didSomething = []() {} // lambda convert to function pointer is good
///     aClass.didSomething = [this]() {} // lambda or functor whose size <= 24 and has trivial destructor is good
template<typename Signature>
class Delegate;

template<typename R, typename... Args>
class Delegate<R(Args...)> {

    typedef Delegate<R(Args...)> Self;

    [[noreturn]] static auto stub_null(void *, Args...) -> R {
        throw Exception("Bad AN::Delegate call");
    }

    typedef R (*stub_function)(void *, Args...);

    stub_function _stub = stub_null;
    void *_obj;
    char _method[16];
public:
    // Creates an unbound delegate
    Delegate() = default;

    Delegate(const Delegate &other)                     = default;
    auto operator=(const Delegate &other) -> Delegate & = default;

    explicit operator bool() const {
        return _stub != stub_null;
    }

    void unBind() {
        _stub = stub_null;
    }


    template<typename Func>
        requires std::is_trivially_destructible_v<Func> &&
                 (!std::is_pointer_v<Func>) &&
                 (sizeof(Func) <= sizeof _method + sizeof _obj) &&
                 std::is_invocable_v<Func, Args...> &&
                 std::same_as<std::invoke_result_t<Func, Args...>, R>
    void bind(Func &&func) {
        // copy or move construct
        new ((void *)&_obj) Func(std::forward<Func>(func));
        _stub      = [](void *self, Args... args) -> R {
            return std::invoke(*(Func *)&(((Self *)self)->_obj), std::forward<Args>(args)...);
        };
    }

    void bind(R(*func)(Args...)) {
        _obj       = (void *)func;
        _stub      = [](void *self, Args... args) -> R {
            return std::invoke((R(*)(Args...))((Self *)self)->_obj, std::forward<Args>(args)...);
        };
    }

    template<typename Func>
        requires (!std::same_as<Self, std::decay_t<Func>>)
    Self &operator = (Func &&func) {
        bind(std::forward<Func>(func));
        return *this;
    }

    template<typename Class>
    auto bind(const Class *cls, R(Class::*method)(Args...) const) -> void {
        _obj = (void *)cls;
        memcpy(_method, &method, sizeof method);
        static_assert(sizeof _method >= sizeof method);
        _stub = [](void *_self, Args... args) -> R {
            Self *self = (Self *)_self;
            return std::invoke(*(R(Class::**)(Args...) const)self->_method, (const Class *)self->_obj, std::forward<Args>(args)...);
        };
    }

    template<typename Class>
    auto bind(Class *cls, R(Class::*method)(Args...)) -> void {
        _obj = cls;
        memcpy(_method, &method, sizeof method);
        static_assert(sizeof _method >= sizeof method);
        _stub = [](void *_self, Args... args) -> R {
            Self *self = (Self *)_self;
            return std::invoke(*(R(Class::**)(Args...))self->_method, (Class *)self->_obj, std::forward<Args>(args)...);
        };
    }


    template<typename... UArgs,
             typename = std::enable_if_t<std::is_invocable_v<R(Args...), UArgs...>>>
    auto operator()(UArgs &&...args) const -> R {
        return _stub((void *)this, std::forward<UArgs>(args)...);
    }
};



}// namespace AN

#endif//OJOIE_DELEGATE_HPP
