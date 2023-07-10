//
// Created by Aleudillonam on 8/16/2022.
//

#ifndef OJOIE_LRUCACHE_HPP
#define OJOIE_LRUCACHE_HPP

#include <optional>
#include <list>
#include <unordered_map>
#include <ranges>

namespace AN {

template<typename K, typename V>
class LRUCache {

    std::list<K> items;
    std::unordered_map<K, std::pair<V, typename std::list<K>::iterator>> keyValuesMap;

public:

    constexpr LRUCache() = default;

    void set(const K &key, const V &value) {
        auto pos = keyValuesMap.find(key);
        if (pos == keyValuesMap.end()) {
            items.push_front(key);
            keyValuesMap[key] = { value, items.begin() };
        } else {
            items.erase(pos->second.second);
            items.push_front(key);
            keyValuesMap[key] = { value, items.begin() };
        }
    }

    bool get(const K &key, V &value) {
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

    void erase(const K &key) {
        auto pos = keyValuesMap.find(key);
        if (pos != keyValuesMap.end()) {
            items.erase(pos->second.second);
            keyValuesMap.erase(pos);
        }
    }

    size_t size() const { return items.size(); }

    /// view like [k, v], unordered
    auto mapView() {
        return keyValuesMap | std::views::transform([](auto &&pair) {
                   return std::make_pair(pair.first, pair.second.first);
               });
    }

    /// view like [k, v], least recently used first order
    auto mapLRUView() {
        return items | std::views::reverse | std::views::transform([this](auto &&key) {
                   return std::make_pair(key, keyValuesMap[key].first);
               });
    }
};


}

#endif//OJOIE_LRUCACHE_HPP
