//
// Created by aojoie on 6/18/2023.
//

#include "App/Views/View.hpp"
#include "Core/CGTypes.hpp"

#ifdef _WIN32
#include "App/Views/win32/View.hpp"
#endif//_WIN32

namespace AN {

View *View::Alloc() {
#ifdef _WIN32
    return new WIN::View();
#else
#error "not implement"
#endif//_WIN32
}



}