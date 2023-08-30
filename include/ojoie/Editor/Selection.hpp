//
// Created by Aleudillonam on 7/9/2023.
//

#pragma once

#include <ojoie/Object/Object.hpp>
#include <ojoie/Core/Actor.hpp>

namespace AN::Editor {

class AN_API Selection {
public:

    static void SetActiveObject(Object *obj);
    static Object *GetActiveObject();
    static Actor *GetActiveActor();

};


}