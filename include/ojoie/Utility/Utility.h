//
// Created by aojoie on 4/11/2023.
//

#ifndef OJOIE_UTILITY_H
#define OJOIE_UTILITY_H

#ifdef __cplusplus

namespace AN {

template<typename DataType>
inline bool CompareArrays(const DataType *lhs, const DataType *rhs, long int arraySize) {
    for (long int i = 0; i < arraySize; i++) {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

template<typename DataType>
inline bool CompareMemory(const DataType &lhs, const DataType &rhs) {
    // We check at compile time if it's safe to cast data to int*
    if constexpr (alignof(DataType) >= alignof(int) && (sizeof(DataType) % sizeof(int)) == 0) {
        return CompareArrays((const int *) &lhs, (const int *) &rhs, sizeof(DataType) / sizeof(int));
    }
    return CompareArrays((const char *) &lhs, (const char *) &rhs, sizeof(DataType));
}

template<typename T>
struct memcmp_less {
    bool operator () (const T& lhs, const T& rhs) const {
        return memcmp(&lhs, &rhs, sizeof(T)) < 0;
    }
};

template<typename T>
struct memcmp_greater {
    bool operator () (const T& lhs, const T& rhs) const {
        return memcmp(&lhs, &rhs, sizeof(T)) > 0;
    }
};

template<typename T>
struct memcmp_equal {
    bool operator () (const T& lhs, const T& rhs) const {
        return memcmp(&lhs, &rhs, sizeof(T)) == 0;
    }
};

}// namespace AN

#endif //__cplusplus

#endif//OJOIE_UTILITY_H
