//
// Created by Aleudillonam on 7/27/2022.
//

#ifndef OJOIE_TASK_HPP
#define OJOIE_TASK_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Utility/Log.h>
#include <memory>
#include <utility>


namespace AN {

class TaskInterface {
    void (*_run)(TaskInterface *super) = nullptr;
    void (*_cancel)(TaskInterface *super) = nullptr;
protected:
    constexpr static int buffer_size = 24;
    char buffer[buffer_size];
public:
    TaskInterface() = default;

    explicit TaskInterface(void (*run)(TaskInterface *super), void (*cancel)(TaskInterface *super))
        : _run(run), _cancel(cancel) {}

    TaskInterface(TaskInterface &&other) noexcept : _run(other._run), _cancel(other._cancel) {
        other._run = nullptr;
        other._cancel = nullptr;
        memcpy(buffer, other.buffer, sizeof(buffer));
    }

    template<typename _SubClass>
        requires (std::is_base_of_v<TaskInterface, _SubClass> && std::is_rvalue_reference_v<_SubClass>)
    TaskInterface(_SubClass &&other) noexcept : _run(other._run), _cancel(other._cancel) {
        other._run = nullptr;
        other._cancel = nullptr;
        memcpy(buffer, other.buffer, sizeof(buffer));
    }

    ~TaskInterface() {
        cancel();
    }

    void run() {
        if (_run) {
            _run(this);
            _cancel = nullptr;
            _run    = nullptr;
        } else {
            AN_LOG(Warning, "%s", "TaskInterface requires concrete run() override,"
                                  " this could occur due to using invalid Task instance");
        }
    }

    void cancel() {
        if (_cancel) {
            _cancel(this);
            _cancel = nullptr;
            _run = nullptr;
        }
    }

    explicit operator bool() const { return _run != nullptr; }

    template<typename Func>
        requires std::is_invocable_v<Func>
    TaskInterface &operator = (Func &&func);

    template<typename _SubClass>
        requires (std::is_base_of_v<TaskInterface, _SubClass> && std::is_rvalue_reference_v<_SubClass>)
    TaskInterface &operator = (_SubClass &&other) noexcept {
        TaskInterface task{ std::move(other) };
        task.swap(*this);
        return *this;
    }

    TaskInterface &operator = (TaskInterface &&other) noexcept {
        other.swap(*this);
        return *this;
    }

    void swap(TaskInterface &other) {
        std::swap(_run, other._run);
        std::swap(_cancel, other._cancel);
        std::swap(buffer, other.buffer);
    }
};

///
/// \attention a taskItem can ether run or cancel
template<typename Func>
class TaskItem : public TaskInterface {
    typedef TaskItem<Func> Self;
    inline constexpr static bool is_inline = TaskInterface::buffer_size >= sizeof(Func);
public:
    using TaskInterface::TaskInterface;
    explicit TaskItem(Func f) : TaskInterface(__run, __cancel) {
        if constexpr (is_inline) {
            // inline move construct
            new ((void*)buffer) Func(std::move(f));
        } else {
            Func **ptr = (Func **)buffer;
            *ptr = new Func(std::move(f));
        }
    }

    static void __cancel(TaskInterface *super) {
        Self *self = (Self *)super;
        if constexpr (is_inline) {
            Func *fn = (Func *)self->buffer;
            std::destroy_at(fn);
        } else {
            Func *fn = *(Func **)self->buffer;
            delete fn;
        }
    }

    /// \override
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

static_assert(sizeof(TaskInterface) == sizeof(TaskItem<void(*)()>));

template<typename Func>
    requires std::is_invocable_v<Func>
TaskInterface &TaskInterface::operator = (Func &&func) {
    (*this) = TaskItem(std::forward<Func>(func));
    return *this;
}

class TaskFence : private NonCopyable {
    std::atomic_flag flag;
public:

    void signal() {
        flag.test_and_set(std::memory_order_release);
        flag.notify_one();
    }

    void wait() {
        flag.wait(false, std::memory_order_acquire);
    }

    bool isReady() {
        return flag.test(std::memory_order_acquire);
    }

    void reset() {
        flag.clear(std::memory_order_relaxed);
    }
};

}

#endif//OJOIE_TASK_HPP
