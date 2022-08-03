//
// Created by Aleudillonam on 7/29/2022.
//

#ifndef OJOIE_TEMPLATE_HPP
#define OJOIE_TEMPLATE_HPP

#include <memory>
#include <unordered_set>

namespace AN {

template<typename T>
struct UniquePtrSetHash {
    using is_transparent = void;

    inline auto operator()(const std::unique_ptr<T>& p) const {
        return std::hash<std::unique_ptr<T>>{}(p);
    }
    inline auto operator()(T* p) const {
        return std::hash<T*>{}(p);
    }
};

template<typename T>
struct UniquePtrSetEqual {
    using is_transparent = void;

    template<typename LHS, typename RHS>
    inline auto operator()(const LHS& lhs, const RHS& rhs) const {
        return AsPtr(lhs) == AsPtr(rhs);
    }
private:
    inline static const T* AsPtr(const T* p) { return p; }
    inline static const T* AsPtr(const std::unique_ptr<T>& p) { return p.get(); }
};

template<typename T>
using UnorderUnquePtrSet = std::unordered_set<std::unique_ptr<T>, UniquePtrSetHash<T>, UniquePtrSetEqual<T>>;

}

#endif//OJOIE_TEMPLATE_HPP
