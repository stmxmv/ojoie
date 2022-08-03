//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_PRIVATE_APP_HPP
#define OJOIE_PRIVATE_APP_HPP

#include "Core/Window.hpp"
#include "Core/Task.hpp"
#include "concurrentqueue/concurrentqueue.hpp"

#include <GLFW/glfw3.h>

#include <vector>

namespace AN {
struct Application::Impl {
    bool isTerminated{};
    std::vector<Window *> windows;
    int numOfVisibleWindows;

    moodycamel::ConcurrentQueue<TaskInterface> dispatchTasks;

};

}


#endif//OJOIE_PRIVATE_APP_HPP
