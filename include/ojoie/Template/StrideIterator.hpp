//
// Created by aojoie on 4/11/2023.
//

#ifndef OJOIE_STRIDEITERATOR_HPP
#define OJOIE_STRIDEITERATOR_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {


template<typename T>
class StrideIterator {
public:
    typedef T                               value_type;
    typedef ptrdiff_t                       difference_type;
    typedef T                              *pointer;
    typedef T                              &reference;
    typedef std::random_access_iterator_tag iterator_category;

    StrideIterator() : m_Pointer(), m_Stride(1) {}
    StrideIterator(void *p, int stride) : m_Pointer(reinterpret_cast<uint8_t *>(p)), m_Stride(stride) {}
    StrideIterator(StrideIterator const &arg) : m_Pointer(arg.m_Pointer), m_Stride(arg.m_Stride) {}

    void operator=(StrideIterator const &arg) {
        m_Pointer = arg.m_Pointer;
        m_Stride  = arg.m_Stride;
    }

    bool operator==(StrideIterator const &arg) const { return m_Pointer == arg.m_Pointer; }
    bool operator!=(StrideIterator const &arg) const { return m_Pointer != arg.m_Pointer; }

    bool operator<(StrideIterator const &arg) const { return m_Pointer < arg.m_Pointer; }

    void operator++() { m_Pointer += m_Stride; }
    void operator++(int) { m_Pointer += m_Stride; }

    StrideIterator operator+(difference_type n) const { return StrideIterator(m_Pointer + m_Stride * n, m_Stride); }
    void           operator+=(difference_type n) { m_Pointer += m_Stride * n; }

    difference_type operator-(StrideIterator const &it) const {
        ANAssert(m_Stride == it.m_Stride && "Iterators stride must be equal");
        ANAssert(m_Stride != 0 && "Stide must not be zero");
        return ((uintptr_t) m_Pointer - (uintptr_t) it.m_Pointer) / m_Stride;
    }

    T       &operator[](size_t index) { return *reinterpret_cast<T *>(m_Pointer + m_Stride * index); }
    const T &operator[](size_t index) const { return *reinterpret_cast<const T *>(m_Pointer + m_Stride * index); }

    T       &operator*() { return *reinterpret_cast<T *>(m_Pointer); }
    const T &operator*() const { return *reinterpret_cast<const T *>(m_Pointer); }

    T       *operator->() { return reinterpret_cast<T *>(m_Pointer); }
    const T *operator->() const { return reinterpret_cast<const T *>(m_Pointer); }

    // Iterator is NULL if not valid
    bool  isNull() const { return m_Pointer == 0; }
    void *getPointer() const { return m_Pointer; }
    int   getStride() const { return m_Stride; }

private:
    uint8_t *m_Pointer;
    int      m_Stride;
};

template<typename T>
void strided_copy(const T *src, const T *srcEnd, StrideIterator<T> dst) {
    for (; src != srcEnd; src++, ++dst)
        *dst = *src;
}

template<typename T>
void strided_copy(StrideIterator<T> src, StrideIterator<T> srcEnd, StrideIterator<T> dst) {
    for (; src != srcEnd; ++src, ++dst)
        *dst = *src;
}

template<typename T>
void strided_copy(StrideIterator<T> src, StrideIterator<T> srcEnd, T *dst) {
    for (; src != srcEnd; ++src, ++dst)
        *dst = *src;
}

template<typename T, typename T2>
void strided_copy_convert(const T *src, const T *srcEnd, StrideIterator<T2> dst) {
    for (; src != srcEnd; ++src, ++dst)
        *dst = *src;
}

}// namespace AN

#endif//OJOIE_STRIDEITERATOR_HPP
