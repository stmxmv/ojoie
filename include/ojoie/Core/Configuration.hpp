//
// Created by Aleudillonam on 8/23/2022.
//

#ifndef OJOIE_CONFIGURATION_HPP
#define OJOIE_CONFIGURATION_HPP

#include "ojoie/Configuration/typedef.h"

#include <ojoie/Template//delegate.hpp>
#include <ojoie/Core/Name.hpp>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <any>
#include <list>

namespace AN {

/// Configuration runs observer on caller thread
class AN_API Configuration : private NonCopyable {
    mutable std::shared_mutex mapMutex;
    std::unordered_map<Name, std::any> configMap;
    typedef std::list<Delegate<void(const std::any&)>> ObserverList;
    std::unordered_map<Name, ObserverList> observers;
    std::unordered_map<void *, std::vector<std::pair<Name, ObserverList::iterator>>> classObservers;
public:

    typedef ObserverList::iterator ObserverID;

    static Configuration &GetSharedConfiguration();

    /// when setting a string value, better not string_view
    void setObject(const char *key, const std::any &obj) {
        std::unique_lock lock(mapMutex);
        ObserverList *delegateList;
        {
            Name name(key);
            configMap[name] = obj;
            delegateList = &observers[name];
        }

        /// notify the observer, observer may call getObject, unlock to avoid deadlock
        for (auto &observer : *delegateList) {
            observer(obj);
        }
    }

    std::any getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        return configMap.at(Name(key));
    }

    template<typename T>
    T getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        Name name(key);
        return std::any_cast<T>(configMap.at(name));
    }

    static const char *GetString(const std::any &object);

    template<typename Func>
    ObserverID addObserver(const char *key, Func &&func) {
        std::unique_lock lock(mapMutex);
        Delegate<void(const std::any&)> observer;
        observer.bind(std::forward<Func>(func));
        auto &obs = observers[Name(key)];
        obs.push_back(observer);
        return --obs.end();
    }

    template<typename _Class, typename Method>
    ObserverID addObserver(const char *key, _Class *self, Method &&method) {
        std::unique_lock lock(mapMutex);
        Delegate<void(const std::any&)> observer;
        observer.bind(self, std::forward<Method>(method));

        Name name(key);
        auto &obs = observers[name];
        obs.push_back(observer);

        classObservers[self].emplace_back(name, --obs.end());
        return --obs.end();
    }

    void removeObserver(const char *key, ObserverID id) {
        std::unique_lock lock(mapMutex);
        observers[Name(key)].erase(std::move(id));
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
        Name name(key);
        for (auto iter = observingList.begin(); iter != observingList.end(); ) {
            if (iter.first == name) {
                observers[name].erase(iter.second);
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
