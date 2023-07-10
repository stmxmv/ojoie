//
// Created by Aleudillonam on 7/29/2022.
//

#ifndef OJOIE_COMPLETIONHANDLER_HPP
#define OJOIE_COMPLETIONHANDLER_HPP

#include <utility>
#include <memory>

namespace AN {


template<typename ...Args>
class CompletionInterface {
    void (*_run)(void *self, Args...);
public:

    virtual ~CompletionInterface() = default;

    explicit CompletionInterface(void (*f)(void *self, Args...)) : _run(f) {}

    void run(Args ...args) {
        _run(this, std::move(args)...);
    }

};

template<typename Func, typename ...Args>
class CompletionHandler : public CompletionInterface<Args...> {
    typedef CompletionHandler<Func, Args...> Self;
    typedef CompletionInterface<Args...> Super;
    Func fn;
public:

    static std::shared_ptr<Self> Alloc(Func &&f) {
        return std::make_shared<Self>(std::forward<Func>(f));
    }

    explicit CompletionHandler(Func f) : Super(__run), fn(std::move(f)) {}

    static void __run(void *super, Args ...args) {
        Self *self = (Self *)super;
        self->fn(std::move(args)...);
    }
};

template <typename T>
struct CompletionHandler_traits;

template <typename T>
struct CompletionHandler_traits
    : public CompletionHandler_traits<decltype(&T::operator())>
{};

template <typename ReturnTypeT, typename... Args> // Free functions
struct CompletionHandler_traits<ReturnTypeT(*)(Args...)> {
    using arguments = std::tuple<Args...>;

    static constexpr std::size_t arity = std::tuple_size<arguments>::value;

    template <std::size_t N>
    using argument_type = typename std::tuple_element<N, arguments>::type;

    using return_type = ReturnTypeT;

    using toCompletionHandler = CompletionHandler<ReturnTypeT(*)(Args...), Args...>;
};

// For generic types, directly use the result of the signature of its 'operator()'
template <typename ClassType, typename ReturnType, typename... Args>
struct CompletionHandler_traits<ReturnType(ClassType::*)(Args...) const> {

    enum { arity = sizeof...(Args) };
    // arity is the number of arguments.

    typedef ReturnType result_type;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        // the i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
    };

    using toCompletionHandler = CompletionHandler<ClassType, Args...>;
};

template<typename Func>
inline auto AllocateCompletionHandler(Func &&func) {
    typedef typename CompletionHandler_traits<std::decay_t<Func>>::toCompletionHandler HandlerType;
    return HandlerType::Alloc(std::forward<Func>(func));
}




}

#endif//OJOIE_COMPLETIONHANDLER_HPP
