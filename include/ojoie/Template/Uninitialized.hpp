//
// Created by aojoie on 10/1/2022.
//

#ifndef OJOIE_UNINITIALIZED_HPP
#define OJOIE_UNINITIALIZED_HPP

namespace AN {

/// \brief a handy template to declare uninitialized c++ class that will not auto-destruct.
/// \note will work if you explicitly cast this template class to T, because they have the same size.
template<typename T>
class Uninitialized {
    typedef Uninitialized<T> Self;

    char memory[sizeof(T)];

public:

    T &get() {
        return *(T *)memory;
    }

    template<typename ...Args>
    void construct(Args &&...args) {
        std::construct_at(reinterpret_cast<T *>(memory), std::forward<Args>(args)...);
    }

    void destruct() {
        std::destroy_at(reinterpret_cast<T *>(memory));
    }

};



}

#endif//OJOIE_UNINITIALIZED_HPP
