//
// Created by Aleudillonam on 8/2/2023.
//

#pragma once

#include <ojoie/Object/Object.hpp>

namespace AN {

class AN_API SerializeManager {

public:

    bool serializeObjectAtPath(Object *object, const char *path);

};

AN_API SerializeManager &GetSerializeManager();

}

