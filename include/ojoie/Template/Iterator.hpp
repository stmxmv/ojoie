//
// Created by aojoie on 10/22/2022.
//

#ifndef OJOIE_ITERATOR_HPP
#define OJOIE_ITERATOR_HPP

#include <cinttypes>

namespace AN {

template<typename T, typename Integer = uint64_t>
class IndexedIterator {
    typedef IndexedIterator<T, Integer> Self;

    T &t;
    Integer index;

public:
    explicit IndexedIterator(T &obj) : t(obj), index() {}
    IndexedIterator(T &obj, Integer idx) : t(obj), index(idx) {}

    auto &operator*() {
        return t[index];
    }

    const auto &operator*() const {
        return t[index];
    }

    auto &operator->() {
        return t[index];
    }

    const auto &operator->() const {
        return t[index];
    }

    Self &operator++() {
        ++index;
        return *this;
    }

    Self operator++(int) {
        Self copy = *this;
        ++(*this);
        return copy;
    }

    bool operator == (const Self &other) const {
        return index == other.index;
    }

    bool operator != (const Self &other) const {
        return index != other.index;
    }

};


template<typename T>
concept IndexedIterable = requires(T t) {
                         t.objectAtIndex(0);
                         { t.count() } -> std::convertible_to<uint64_t>;
                     };

template<typename T>
class IndexedIteratorImpl {
    typedef IndexedIteratorImpl<T> Self;

    IndexedIterable auto &self() {
        return static_cast<T&>(*this);
    }

public:

    auto &operator[](uint64_t index) {
        return self().objectAtIndex(index);
    }

    const auto &operator[](uint64_t index) const {
        return self().objectAtIndex(index);
    }

    uint64_t size() const {
        return (uint64_t)this->count();
    }

    auto begin() {
        return AN::IndexedIterator<Self>(*this);
    }

    auto end() {
        return AN::IndexedIterator<Self>(*this, self().count());
    }

    auto begin() const {
        return AN::IndexedIterator<const Self>(*this);
    }

    auto end() const {
        return AN::IndexedIterator<const Self>(*this, self().count());
    }

};


}

#endif//OJOIE_ITERATOR_HPP
