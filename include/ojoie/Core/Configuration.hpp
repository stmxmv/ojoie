//
// Created by Aleudillonam on 8/23/2022.
//

#ifndef OJOIE_CONFIGURATION_HPP
#define OJOIE_CONFIGURATION_HPP

#include <ojoie/Core/typedef.h>
#include <ojoie/Core/delegate.hpp>
#include <shared_mutex>
#include <any>
#include <unordered_map>
#include <list>
#include <utility>

namespace AN {


class Configuration : private NonCopyable {
    mutable std::shared_mutex mapMutex;
    std::unordered_map<size_t, std::any> configMap;
    typedef std::list<Delegate<void(const std::any&)>> ObserverList;
    std::unordered_map<size_t, ObserverList> observers;
    std::unordered_map<void *, std::vector<std::pair<size_t, ObserverList::iterator>>> classObservers;
public:

    typedef ObserverList::iterator ObserverID;

    static Configuration &GetSharedConfiguration();

    void setObject(const char *key, const std::any &obj) {
        size_t hash;
        ObserverList *delegateList;
        {
            std::unique_lock lock(mapMutex);
            hash = std::hash<std::string_view>{}(key);
            configMap[hash] = obj;
            delegateList = &observers[hash];
        }

        /// notify the observer, observer may call getObject, unlock to avoid deadlock
        for (auto &observer : *delegateList) {
            observer(obj);
        }
    }

    std::any getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        size_t hash = std::hash<std::string_view>{}(key);
        return configMap.at(hash);
    }

    template<typename T>
    T getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        size_t hash = std::hash<std::string_view>{}(key);
        return std::any_cast<T>(configMap.at(hash));
    }

    template<typename Func>
    ObserverID addObserver(const char *key, Func &&func) {
        std::unique_lock lock(mapMutex);
        Delegate<void(const std::any&)> observer;
        observer.bind(std::forward<Func>(func));
        size_t hash = std::hash<std::string_view>{}(key);
        auto &obs = observers[hash];
        obs.push_back(observer);
        return --obs.end();
    }

    template<typename _Class, typename Method>
    ObserverID addObserver(const char *key, _Class *self, Method &&method) {
        std::unique_lock lock(mapMutex);
        Delegate<void(const std::any&)> observer;
        observer.bind(self, std::forward<Method>(method));
        size_t hash = std::hash<std::string_view>{}(key);
        auto &obs = observers[hash];
        obs.push_back(observer);

        classObservers[self].emplace_back(hash, --obs.end());
        return --obs.end();
    }

    void removeObserver(const char *key, ObserverID id) {
        std::unique_lock lock(mapMutex);
        size_t hash = std::hash<std::string_view>{}(key);
        observers[hash].erase(std::move(id));
    }

    template<typename _Class>
    void removeObserver(_Class *obj) {
        std::unique_lock lock(mapMutex);
        auto &observingList = classObservers[obj];
        for (auto &[hash, iter] : observingList) {
            observers[hash].erase(iter);
        }
        classObservers.erase(obj);
    }

    template<typename _Class>
    void removeObserver(const char *key, _Class *obj) {
        std::unique_lock lock(mapMutex);
        auto &observingList = classObservers[obj];
        size_t hashKey = std::hash<std::string_view>{}(key);
        for (auto iter = observingList.begin(); iter != observingList.end(); ) {
            if (iter.first == hashKey) {
                observers[hashKey].erase(iter.second);
                iter = observingList.erase(iter);
            } else {
                ++iter;
            }
        }
        if (observingList.empty()) {
            classObservers.erase(obj);
        }
    }


};

inline Configuration &GetConfiguration() {
    return Configuration::GetSharedConfiguration();
}

}

#endif//OJOIE_CONFIGURATION_HPP
