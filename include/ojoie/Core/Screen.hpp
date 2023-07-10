//
// Created by aojoie on 4/23/2023.
//

#ifndef OJOIE_SCREEN_HPP
#define OJOIE_SCREEN_HPP

#include <ojoie/Core/CGTypes.hpp>

namespace AN {

class Screen {
public:

    virtual Size getSize() = 0;

    virtual int getRefreshRate() = 0;

};

/// get the default screen
AN_API Screen &GetScreen();

}

#endif//OJOIE_SCREEN_HPP
