//
// Created by Aleudillonam on 8/23/2022.
//

#include "Core/Configuration.hpp"

namespace AN {


Configuration &Configuration::GetSharedConfiguration() {
    static Configuration configuration;
    return configuration;
}

const char *Configuration::GetString(const std::any &object) {
    if (const char *const *ret = std::any_cast<const char *>(&object)) {
        return *ret;
    }
    if (const std::string *ret = std::any_cast<std::string>(&object)) {
        return ret->c_str();
    }
    if (const Name *ret = std::any_cast<Name>(&object)) {
        return ret->c_str();
    }
    return nullptr;
}

}