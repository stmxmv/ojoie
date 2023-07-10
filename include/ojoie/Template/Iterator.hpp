//
// Created by aojoie on 10/22/2022.
//

#ifndef OJOIE_ITERATOR_HPP
#define OJOIE_ITERATOR_HPP

#include <cinttypes>

namespace AN {

/// \brief iterator which use subscript of an array like object
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

    bool operator==(const Self &other) const {
        return index == other.index;
    }

    bool operator!=(const Self &other) const {
        return index != other.index;
    }
};


/// \brief concept which defines primitive method for a constant array object
template<typename T>
concept ConstantArrayConcept = requires(T t) {
    t.objectAtIndex(0);
    { t.count() } -> std::convertible_to<uint64_t>;
};

/// \brief CRTP subclass must satisfy ConstantArrayConcept
template<typename T>
class ConstantArrayBase {
    typedef ConstantArrayBase<T> Self;

    ConstantArrayConcept auto &self() {
        return static_cast<T &>(*this);
    }

public:
    auto &operator[](uint64_t index) {
        return self().objectAtIndex(index);
    }

    const auto &operator[](uint64_t index) const {
        return self().objectAtIndex(index);
    }

    uint64_t size() const {
        return (uint64_t) this->count();
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

/// \brief Allows for iterating a linked list, even if you add / remove any node during traversal.
template<typename T>
class SafeIterator {
    typedef SafeIterator<T> Self;
    typedef typename T::value_type value_type;

    T m_ExecuteList;
    T &m_SourceList;
    value_type *m_CurrentNode;

public:

    explicit SafeIterator(T &list)
        : m_SourceList(list), m_CurrentNode() {
        m_ExecuteList.swap(m_SourceList);
    }

    ~SafeIterator() {
        // Call Complete if you abort the iteration!
        ANAssert(m_ExecuteList.empty());
    }

    // You must call complete if you are in some way aborting list iteration.
    // If you dont call Complete, the source list will lose nodes that have not yet been iterated permanently.
    //
    /// SafeIterator<Behaviour*> i(myList);
    /// i =0;
    /// while(i.GetNext() && ++i != 3)
    ///   (**i).Update();
    /// i.Complete();
    void complete() {
        m_SourceList.splice(m_SourceList.end(), m_ExecuteList);
    }

    Self &next() {
        if (!m_ExecuteList.empty()) {
            typename T::iterator it = m_ExecuteList.begin();
            m_CurrentNode           = &*it;
            m_ExecuteList.erase(it);
            m_SourceList.push_back(*m_CurrentNode);
        } else {
            m_CurrentNode = nullptr;
        }
        return *this;
    }

    value_type &operator*() const { return *m_CurrentNode; }
    value_type *operator->() const { return m_CurrentNode; }

    explicit operator bool() const {
        return m_CurrentNode != nullptr;
    }
};

}// namespace AN

#endif//OJOIE_ITERATOR_HPP
