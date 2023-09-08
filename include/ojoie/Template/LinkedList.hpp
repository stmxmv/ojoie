//
// Created by aojoie on 4/8/2023.
//

#pragma once

#include <ojoie/Utility/Assert.h>

namespace AN {


#ifdef AN_DEBUG
#define LINKED_LIST_ASSERT(x) ANAssert(x)
#else
#define LINKED_LIST_ASSERT(x)
#endif

class ListNodeBase {
    ListNodeBase *prev;
    ListNodeBase *next;

    template<typename T>
    friend class List;

public:
    ListNodeBase() : prev(), next() {}
    ~ListNodeBase() { removeFromList(); }

    ListNodeBase(const ListNodeBase &)             = delete;
    ListNodeBase &operator= (const ListNodeBase &) = delete;

    bool isInList() const {
        return prev != nullptr;
    }

    bool removeFromList() {
        if (!isInList())
            return false;

        prev->next = next;
        next->prev = prev;
        prev       = nullptr;
        next       = nullptr;
        return true;
    }

    void insertInList(ListNodeBase *pos) {
        if (this == pos)
            return;

        if (isInList())
            removeFromList();

        prev       = pos->prev;
        next       = pos;
        prev->next = this;
        next->prev = this;
    }

    ListNodeBase *getPrev() const { return prev; }
    ListNodeBase *getNext() const { return next; }
};

template<typename T>
class ListNode : public ListNodeBase {
    T *data;

public:
    ListNode(T *data = nullptr) : data(data) {}
    T   &operator* () const { return *data; }
    T   *operator->() const { return data; }
    T   *getData() const { return data; }
    void setData(T *aData) { data = aData; }

    // We know the type of prev and next element
    ListNode *getPrev() const { return static_cast<ListNode *>(ListNodeBase::getPrev()); }
    ListNode *getNext() const { return static_cast<ListNode *>(ListNodeBase::getNext()); }
};

template<typename T>
class ListIterator {

    ListNodeBase *node;

    template<typename S>
    friend class List;

public:

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = value_type*;
    using reference         = value_type&;

    explicit ListIterator(T *node = nullptr) : node((ListNodeBase *) node) {}

    ListIterator &operator++ () {
        node = node->getNext();
        return *this;
    }

    ListIterator operator++ (int) {
        ListIterator ret(*this);
        ++(*this);
        return ret;
    }

    ListIterator &operator-- () {
        node = node->getPrev();
        return *this;
    }

    ListIterator operator-- (int) {
        ListIterator ret(*this);
        --(*this);
        return ret;
    }

    T &operator* () const { return static_cast<T &>(*node); }
    T *operator->() const { return static_cast<T *>(node); }

    friend bool operator!= (const ListIterator &x, const ListIterator &y) { return x.node != y.node; }
    friend bool operator== (const ListIterator &x, const ListIterator &y) { return x.node == y.node; }
};

template<typename T>
class List {

    ListNodeBase root;

public:
    typedef ListIterator<const T> const_iterator;
    typedef ListIterator<T>       iterator;
    typedef T                     value_type;

    List() {
        root.prev = &root;
        root.next = &root;
    }

    ~List() {
        clear();
    }

    void push_back(T &node) { node.insertInList(&root); }
    void push_front(T &node) { node.insertInList(root.next); }
    void insert(iterator pos, T &node) { node.insertInList(&(*pos)); }
    void erase(iterator pos) { pos->removeFromList(); }

    void pop_back() {
        if (root.prev != &root) root.prev->removeFromList();
    }
    void pop_front() {
        if (root.next != &root) root.next->removeFromList();
    }

    iterator begin() { return iterator{ (T*)root.next }; }
    iterator end() { return iterator{ (T*)&root }; }

    const_iterator begin() const { return const_iterator{ (T*)root.next }; }
    const_iterator end() const { return const_iterator{ (T*)&root }; }

    T &front() {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<T &>(*root.next);
    }
    T &back() {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<T &>(*root.prev);
    }

    const T &front() const {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<const T &>(*root.next);
    }
    const T &back() const {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<const T &>(*root.prev);
    }

    bool empty() const { return begin() == end(); }

    size_t size_slow() const {
        size_t        size = 0;
        ListNodeBase *node = root.next;
        while (node != &root) {
            node = node->next;
            size++;
        }
        return size;
    }

    void clear() {
        ListNodeBase *node = root.next;
        while (node != &root) {
            ListNodeBase *next = node->next;
            node->prev         = nullptr;
            node->next         = nullptr;
            node               = next;
        }
        root.next = &root;
        root.prev = &root;
    }

    inline void swap(List &other) {
        LINKED_LIST_ASSERT(this != &other);

        std::swap(other.root.prev, root.prev);
        std::swap(other.root.next, root.next);

        if (other.root.prev == &root)
            other.root.prev = &other.root;
        if (root.prev == &other.root)
            root.prev = &root;
        if (other.root.next == &root)
            other.root.next = &other.root;
        if (root.next == &other.root)
            root.next = &root;

        other.root.prev->next = &other.root;
        other.root.next->prev = &other.root;

        root.prev->next = &root;
        root.next->prev = &root;
    }

    // Insert list into list (removes elements from source)
    void splice(iterator pos, List &src) {
        LINKED_LIST_ASSERT(this != &src);
        if (src.empty())
            return;

        // Insert source before pos
        ListNodeBase *a = pos.node->prev;
        ListNodeBase *b = pos.node;
        a->next         = src.root.next;
        b->prev         = src.root.prev;
        a->next->prev   = a;
        b->prev->next   = b;
        // Clear source list
        src.root.next = &src.root;
        src.root.prev = &src.root;
    }

    void append(List &src) {
        splice(end(), src);
    }
};

}// namespace AN