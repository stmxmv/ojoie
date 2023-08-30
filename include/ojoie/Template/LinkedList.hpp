//
// Created by aojoie on 4/8/2023.
//

#ifndef OJOIE_LINKEDLIST_HPP
#define OJOIE_LINKEDLIST_HPP

#include <ojoie/Utility/Assert.h>

namespace AN {


#ifdef AN_DEBUG
#define LINKED_LIST_ASSERT(x) ANAssert(x)
#else
#define LINKED_LIST_ASSERT(x)
#endif

class ListElement {
    ListElement *m_Prev;
    ListElement *m_Next;
    
    template<typename T>
    friend class List;
public:
    
    ListElement() : m_Prev(), m_Next() {}
    ~ListElement() { removeFromList(); }

    ListElement(const ListElement &)             = delete;
    ListElement &operator= (const ListElement &) = delete;
    
    inline bool isInList() const;
    inline bool removeFromList();
    inline void insertInList(ListElement *pos);

    ListElement *getPrev() const { return m_Prev; }
    ListElement *getNext() const { return m_Next; }
};

template<typename T>
class ListNode : public ListElement {
public:
    ListNode(T *data = nullptr) : m_Data(data) {}
    T   &operator* () const { return *m_Data; }
    T   *operator->() const { return m_Data; }
    T   *GetData() const { return m_Data; }
    void SetData(T *data) { m_Data = data; }

    // We know the type of prev and next element
    ListNode *getPrev() const { return static_cast<ListNode *>(ListElement::getPrev()); }
    ListNode *getNext() const { return static_cast<ListNode *>(ListElement::getNext()); }

private:
    T *m_Data;
};

template<typename T>
class ListIterator {
public:
    ListIterator(T *node = nullptr) : m_Node(node) {}

    // Pre- and post-increment operator
    ListIterator &operator++ () {
        m_Node = m_Node->getNext();
        return *this;
    }
    ListIterator operator++ (int) {
        ListIterator ret(*this);
        ++(*this);
        return ret;
    }

    // Pre- and post-decrement operator
    ListIterator &operator-- () {
        m_Node = m_Node->getPrev();
        return *this;
    }
    ListIterator operator-- (int) {
        ListIterator ret(*this);
        --(*this);
        return ret;
    }

    T &operator* () const { return static_cast<T &>(*m_Node); }
    T *operator->() const { return static_cast<T *>(m_Node); }

    friend bool operator!= (const ListIterator &x, const ListIterator &y) { return x.m_Node != y.m_Node; }
    friend bool operator== (const ListIterator &x, const ListIterator &y) { return x.m_Node == y.m_Node; }

private:
    template<typename S>
    friend class List;
    ListIterator(ListElement *node) : m_Node(node) {}
    ListElement *m_Node;
};


template<typename T>
class ListConstIterator {
public:
    ListConstIterator(const T *node = nullptr) : m_Node(node) {}

    // Pre- and post-increment operator
    ListConstIterator &operator++ () {
        m_Node = m_Node->getNext();
        return *this;
    }
    ListConstIterator operator++ (int) {
        ListConstIterator ret(*this);
        ++(*this);
        return ret;
    }

    // Pre- and post-decrement operator
    ListConstIterator &operator-- () {
        m_Node = m_Node->getPrev();
        return *this;
    }
    ListConstIterator operator-- (int) {
        ListConstIterator ret(*this);
        --(*this);
        return ret;
    }

    const T &operator* () const { return static_cast<const T &>(*m_Node); }
    const T *operator->() const { return static_cast<const T *>(m_Node); }

    friend bool operator!= (const ListConstIterator &x, const ListConstIterator &y) { return x.m_Node != y.m_Node; }
    friend bool operator== (const ListConstIterator &x, const ListConstIterator &y) { return x.m_Node == y.m_Node; }

private:
    template<typename S>
    friend class List;
    ListConstIterator(const ListElement *node) : m_Node(node) {}
    const ListElement *m_Node;
};

template<typename T>
class List {
public:
    typedef ListConstIterator<T> const_iterator;
    typedef ListIterator<T>      iterator;
    typedef T                    value_type;

    inline List();
    inline ~List();

    void push_back(T &node) { node.insertInList(&m_Root); }
    void push_front(T &node) { node.insertInList(m_Root.m_Next); }
    void insert(iterator pos, T &node) { node.insertInList(&(*pos)); }
    void erase(iterator pos) { pos->removeFromList(); }

    void pop_back() {
        if (m_Root.m_Prev != &m_Root) m_Root.m_Prev->removeFromList();
    }
    void pop_front() {
        if (m_Root.m_Next != &m_Root) m_Root.m_Next->removeFromList();
    }

    iterator begin() { return iterator(m_Root.m_Next); }
    iterator end() { return iterator(&m_Root); }

    const_iterator begin() const { return const_iterator(m_Root.m_Next); }
    const_iterator end() const { return const_iterator(&m_Root); }

    T &front() {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<T &>(*m_Root.m_Next);
    }
    T &back() {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<T &>(*m_Root.m_Prev);
    }

    const T &front() const {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<const T &>(*m_Root.m_Next);
    }
    const T &back() const {
        LINKED_LIST_ASSERT(!empty());
        return static_cast<const T &>(*m_Root.m_Prev);
    }

    bool empty() const { return begin() == end(); }

    size_t      size_slow() const;
    inline void clear();
    inline void swap(List &other);

    // Insert list into list (removes elements from source)
    inline void splice(iterator pos, List &src);
    inline void append(List &src);

private:
    ListElement m_Root;
};


template<typename T>
List<T>::List() {
    m_Root.m_Prev = &m_Root;
    m_Root.m_Next = &m_Root;
}

template<typename T>
List<T>::~List() {
    clear();
}

template<typename T>
size_t List<T>::size_slow() const {
    size_t       size = 0;
    ListElement *node = m_Root.m_Next;
    while (node != &m_Root) {
        node = node->m_Next;
        size++;
    }
    return size;
}

template<typename T>
void List<T>::clear() {
    ListElement *node = m_Root.m_Next;
    while (node != &m_Root) {
        ListElement *next = node->m_Next;
        node->m_Prev      = nullptr;
        node->m_Next      = nullptr;
        node = next;
    }
    m_Root.m_Next = &m_Root;
    m_Root.m_Prev = &m_Root;
}

template<typename T>
void List<T>::swap(List<T> &other) {
    LINKED_LIST_ASSERT(this != &other);

    std::swap(other.m_Root.m_Prev, m_Root.m_Prev);
    std::swap(other.m_Root.m_Next, m_Root.m_Next);

    if (other.m_Root.m_Prev == &m_Root)
        other.m_Root.m_Prev = &other.m_Root;
    if (m_Root.m_Prev == &other.m_Root)
        m_Root.m_Prev = &m_Root;
    if (other.m_Root.m_Next == &m_Root)
        other.m_Root.m_Next = &other.m_Root;
    if (m_Root.m_Next == &other.m_Root)
        m_Root.m_Next = &m_Root;

    other.m_Root.m_Prev->m_Next = &other.m_Root;
    other.m_Root.m_Next->m_Prev = &other.m_Root;

    m_Root.m_Prev->m_Next = &m_Root;
    m_Root.m_Next->m_Prev = &m_Root;
}

template<typename T>
void List<T>::splice(iterator pos, List<T> &src) {
    LINKED_LIST_ASSERT(this != &src);
    if (src.empty())
        return;

    // Insert source before pos
    ListElement *a    = pos.m_Node->m_Prev;
    ListElement *b    = pos.m_Node;
    a->m_Next         = src.m_Root.m_Next;
    b->m_Prev         = src.m_Root.m_Prev;
    a->m_Next->m_Prev = a;
    b->m_Prev->m_Next = b;
    // Clear source list
    src.m_Root.m_Next = &src.m_Root;
    src.m_Root.m_Prev = &src.m_Root;
}

template<typename T>
void List<T>::append(List &src) {
    splice(end(), src);
}


bool ListElement::isInList() const {
    return m_Prev != nullptr;
}

bool ListElement::removeFromList() {
    if (!isInList())
        return false;

    m_Prev->m_Next = m_Next;
    m_Next->m_Prev = m_Prev;
    m_Prev         = nullptr;
    m_Next         = nullptr;
    return true;
}

void ListElement::insertInList(ListElement *pos) {
    if (this == pos)
        return;

    if (isInList())
        removeFromList();

    m_Prev         = pos->m_Prev;
    m_Next         = pos;
    m_Prev->m_Next = this;
    m_Next->m_Prev = this;
}


}// namespace AN

#endif//OJOIE_LINKEDLIST_HPP
