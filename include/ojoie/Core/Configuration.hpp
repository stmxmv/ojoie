//
// Created by Aleudillonam on 8/23/2022.
//

#ifndef OJOIE_CONFIGURATION_HPP
#define OJOIE_CONFIGURATION_HPP

#include <ojoie/Core/typedef.h>
#include <shared_mutex>
#include <any>
#include <unordered_map>

namespace AN {


class Configuration : private NonCopyable {
    mutable std::shared_mutex mapMutex;
    std::unordered_map<std::string_view, std::any> configMap;
public:

    static Configuration &GetSharedConfiguration();

    void setObject(const char *key, const std::any &obj) {
        std::unique_lock lock(mapMutex);
        configMap[key] = obj;
    }

    std::any getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        return configMap.at(key);
    }

    template<typename T>
    T getObject(const char *key) const {
        std::shared_lock lock(mapMutex);
        return std::any_cast<T>(configMap.at(key));
    }

};

inline Configuration &GetConfiguration() {
    return Configuration::GetSharedConfiguration();
}

}

#endif//OJOIE_CONFIGURATION_HPP
