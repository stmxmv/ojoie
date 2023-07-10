//
// Created by aojoie on 4/1/2023.
//

#ifndef OJOIE_OBJECTPTR_HPP
#define OJOIE_OBJECTPTR_HPP

#include <ojoie/Object/Object.hpp>

namespace AN {

template<typename T>
class ObjectPtr {

    T *obj;

public:

    ObjectPtr() : obj() {}

    explicit ObjectPtr(T *obj) : obj(obj) {}

    ObjectPtr(const ObjectPtr &) = delete;

    ObjectPtr(ObjectPtr &&other) noexcept : obj(other.obj) {
        other.detach();
    }

    template<typename U>
    explicit ObjectPtr(ObjectPtr<U> &&other) noexcept {
        if (other) {
            obj = other->template as<T>();
            if (obj) {
                other.detach();
            }
        } else {
            obj = nullptr;
        }
    }

    ObjectPtr &operator = (std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    ObjectPtr &operator = (ObjectPtr &&other) noexcept {
        other.swap(*this);
        return *this;
    }

    template<typename U>
    ObjectPtr &operator = (ObjectPtr<U> &&other) noexcept {
        std::destroy_at(this);
        if (other) {
            obj = other->template as<T>();
            if (obj) {
                other.detach();
            }
        }
        return *this;
    }

    ~ObjectPtr() {
       reset();
    }

    T * operator ->() {
        ANAssert(obj, "Trying to dereference a null ObjectPtr");
        return obj;
    }

    T * operator ->() const {
        ANAssert(obj, "Trying to dereference a null ObjectPtr");
        return obj;
    }

    T *get() const {
        return obj;
    }

    T *detach() {
        T *ret = obj;
        obj = nullptr;
        return ret;
    }

    void reset() {
        if (obj) {
            DestroyObject(obj);
            obj = nullptr;
        }
    }

    void swap(ObjectPtr &other) {
        std::swap(obj, other.obj);
    }

    explicit operator bool() const {
        return obj != nullptr;
    }

    bool operator == (std::nullptr_t) const {
        return obj == nullptr;
    }

    bool operator != (std::nullptr_t) const {
        return !(*this == nullptr);
    }

};

template<typename T, typename U>
ObjectPtr<T> dyn_cast_transfer(ObjectPtr<U> &ptr) {
    return ObjectPtr<T>(std::move(ptr));
}

template<typename T, typename U>
ObjectPtr<T> dyn_cast_transfer(ObjectPtr<U> &&ptr) {
    return ObjectPtr<T>(std::move(ptr));
}

template<typename T, typename U>
ObjectPtr<T> unsafe_cast_transfer(ObjectPtr<U> &ptr) {
    if (!ptr) return {};
    T *obj = ptr->template asUnsafe<T>();
    ptr.detach();
    return ObjectPtr<T>(obj);
}

template<typename T, typename U>
ObjectPtr<T> unsafe_cast_transfer(ObjectPtr<U> &&ptr) {
    if (!ptr) return {};
    T *obj = ptr->template asUnsafe<T>();
    ptr.detach();
    return ObjectPtr<T>(obj);
}



template<typename T>
inline ObjectPtr<T> MakeObjectPtr() {
    return ObjectPtr(NewObject<T>());
}

/// create a ObjectPtr with class id, return nullptr if cast fail
template<typename T>
inline ObjectPtr<T> MakeObjectPtr(int id) {
    Object *obj = NewObject(id);
    if (!obj) {
        DestroyObject(obj);
        return {};
    }
    T *casted = obj->as<T>();
    if (!casted) {
        DestroyObject(obj);
        return {};
    }
    return ObjectPtr<T>(casted);
}

template<typename T>
inline ObjectPtr<T> MakeObjectPtr(std::string_view name) {
    Object *obj = NewObject(name);
    if (!obj) {
        DestroyObject(obj);
        return {};
    }
    T *casted = obj->as<T>();
    if (!casted) {
        DestroyObject(obj);
        return {};
    }
    return ObjectPtr<T>(casted);
}

template<typename T>
inline ObjectPtr<T> MakeObjectPtrUnsafe(int id) {
    return ObjectPtr<T>((T *) NewObject(id));
}

template<typename T>
inline ObjectPtr<T> MakeObjectPtrUnsafe(std::string_view name) {
    return ObjectPtr<T>((T *) NewObject(name));
}

}

#endif//OJOIE_OBJECTPTR_HPP
