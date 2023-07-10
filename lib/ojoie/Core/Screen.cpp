//
// Created by aojoie on 4/23/2023.
//

#include "Core/Screen.hpp"

#ifdef _WIN32
#include "Core/private/win32/Screen.hpp"
#endif

namespace AN {


Screen &GetScreen() {
#ifdef _WIN32
    static WIN::Screen screen;
#endif
    return screen;
}


}