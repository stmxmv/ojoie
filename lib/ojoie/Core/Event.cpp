//
// Created by aojoie on 5/19/2023.
//

#include "Core/Event.hpp"

namespace AN {


Event &Event::Current() {
    static Event event;
    return event;
}

}