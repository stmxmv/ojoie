//
// Created by Aleudillonam on 8/16/2022.
//

#ifndef OJOIE_LRUCACHE_HPP
#define OJOIE_LRUCACHE_HPP

#include <optional>
#include <list>
#include <unordered_map>

namespace AN {

template<typename K, typename V>
class LRUCache {

    int capacity;
    std::list<K> items;
    std::unordered_map <K, std::pair<V, typename std::list<K>::iterator>> keyValuesMap;


public:

    constexpr LRUCache(int s) : capacity(s) {}

    /// \result deleted least used value
    std::optional<V> set(const K key, const V &value) {
        auto pos = keyValuesMap.find(key);
        if (pos == keyValuesMap.end()) {
            items.push_front(key);
            keyValuesMap[key] = { value, items.begin() };
            if (keyValuesMap.size() > capacity) {
                V deleteValue = std::move(keyValuesMap[items.back()].first);
                keyValuesMap.erase(items.back());
                items.pop_back();
                return deleteValue;
            }
        } else {
            items.erase(pos->second.second);
            items.push_front(key);
            keyValuesMap[key] = { value, items.begin() };
        }
        return {};
    }

    bool get(const K key, V &value) {
        auto pos = keyValuesMap.find(key);
        if (pos == keyValuesMap.end())
            return false;
        items.erase(pos->second.second);
        items.push_front(key);
        keyValuesMap[key] = { pos->second.first, items.begin() };
        value = pos->second.first;
        return true;
    }

    void clear() {
        items.clear();
        keyValuesMap.clear();
    }
};





}

#endif//OJOIE_LRUCACHE_HPP
