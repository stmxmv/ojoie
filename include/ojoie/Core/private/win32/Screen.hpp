//
// Created by aojoie on 4/23/2023.
//

#ifndef OJOIE_WIN32_SCREEN_HPP
#define OJOIE_WIN32_SCREEN_HPP

#include <ojoie/Core/Screen.hpp>
#include <Windows.h>

namespace AN::WIN {

class Screen : public AN::Screen {

public:

    virtual Size getSize() override;

    virtual int  getRefreshRate() override;

};


}

#endif//OJOIE_WIN32_SCREEN_HPP
