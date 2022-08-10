//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_TASK_HPP
#define OJOIE_TASK_HPP

#include <ojoie/Core/Exception.hpp>
#include <utility>
#include <memory>

namespace AN {

class TaskInterface {
    static void _run_dull(TaskInterface *) {
        throw Exception("TaskInterface requires concrete run override");
    }
    void (*_run)(TaskInterface *super) = _run_dull;
protected:
    constexpr static int buffer_size = 24;
    char buffer[buffer_size];
public:
    TaskInterface() = default;

    explicit TaskInterface(void (*f)(TaskInterface *super)) : _run(f) {}

    void run() {
        _run(this);
    }

    explicit operator bool() const {
        return _run != _run_dull;
    }

    template<typename Func>
        requires std::is_invocable_v<Func>
    TaskInterface &operator = (Func &&func);

};

///
/// \attention a taskItem can ether run or cancel
template<typename Func>
class TaskItem : public TaskInterface {
    typedef TaskItem<Func> Self;
    inline constexpr static bool is_inline = TaskInterface::buffer_size >= sizeof(Func);
public:
    using TaskInterface::TaskInterface;
    explicit TaskItem(Func f) : TaskInterface(__run) {
        if constexpr (is_inline) {
            // inline move construct
            new ((void*)buffer) Func(std::move(f));
        } else {
            Func **ptr = (Func **)buffer;
            *ptr = new Func(std::move(f));
        }
    }

    void cancel() {
        if constexpr (is_inline) {
            Func *fn = (Func *)buffer;
            std::destroy_at(fn);
        } else {
            Func *fn = *(Func **)buffer;
            delete fn;
        }
    }

    /// @override
    static void __run(TaskInterface *super) {
        Self *self = (Self *)super;
        if constexpr (is_inline) {
            Func *fn = (Func *)self->buffer;
            (*fn)();
            std::destroy_at(fn);
        } else {
            Func *fn = *(Func **)self->buffer;
            (*fn)();
            delete fn;
        }
    }
};


template<typename Func>
    requires std::is_invocable_v<Func>
TaskInterface &TaskInterface::operator = (Func &&func) {
    (*this) = TaskItem(std::forward<Func>(func));
    return *this;
}

class TaskFence {
    std::atomic_flag flag;
public:

    void signal() {
        flag.test_and_set(std::memory_order_release);
        flag.notify_one();
    }

    void wait() {
        flag.wait(false, std::memory_order_acquire);
    }
};

}

#endif//OJOIE_TASK_HPP
