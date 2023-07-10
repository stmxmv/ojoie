#pragma once


#include <ojoie/Object/Object.hpp>

#include <unordered_map>

namespace AN {

class AN_API ResourceManager {

    std::unordered_map<std::string, Object *> resourceMap;

public:

    void loadBuiltinResources();

    /// if resource with specific className and name exist, this method will unload the existing resource
    Object *loadResource(const char *className, const char *name, const char *searchPath = nullptr);

    Object* getResource(const char *className, const char *name);

};

AN_API ResourceManager &GetResourceManager();

}