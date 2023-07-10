//
// Created by aojoie on 4/12/2023.
//

#include "Core/Name.hpp"
#include "Utility/Assert.h"
#include "Allocator/MemoryDefines.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <shared_mutex>

namespace AN {

class NameManager {
    std::shared_mutex sm;
    /// not use std::string because vector resize the underlying char pointer may change
    std::vector<const char *> _namePool;
    std::unordered_map<std::string_view, size_t> _namePoolMap;
public:

    ~NameManager() {
        for (const char *str : _namePool) {
            AN_FREE((void *)str);
        }
        _namePool.clear();
    }

    size_t getIndex(std::string_view name) {
        {
            std::shared_lock lock(sm);
            if (auto it = _namePoolMap.find(name); it != _namePoolMap.end()) {
                return it->second;
            }
        }

        std::unique_lock lock(sm);
        char *str = (char *)AN_CALLOC_ALIGNED(name.size() + 1, sizeof(char), alignof(char));
        memcpy(str, name.data(), name.size());
        _namePool.emplace_back(str);
        _namePoolMap[_namePool.back()] = _namePool.size() - 1;
        return _namePoolMap.size() - 1;
    }

    const char *getCString(size_t index) {
        /// empty string
        if (index == -1) return "";

        std::shared_lock lock(sm);
        ANAssert(_namePool.size() - 1 >= index && index >= 0);
        auto it = _namePool.begin();
        std::advance(it, index);
        return *it;
    }
};

NameManager &GetNameManager() {
    static NameManager nameManager;
    return nameManager;
}

Name::Name(std::string_view str) {
    index = GetNameManager().getIndex(str);
    _id = GetNameManager().getCString(index);
}

Name::Name(const char *str) : Name(std::string_view{str}) {}

Name &Name::operator= (const char *str) {
    index = GetNameManager().getIndex(str);
    _id = GetNameManager().getCString(index);
    return *this;
}

Name &Name::operator= (std::string_view str) {
    index = GetNameManager().getIndex(str);
    _id = GetNameManager().getCString(index);
    return *this;
}

}