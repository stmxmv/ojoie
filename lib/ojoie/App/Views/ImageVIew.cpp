//
// Created by aojoie on 6/18/2023.
//

#include "App/Views/ImageView.hpp"

#ifdef _WIN32
#include "App/Views/win32/ImageVIew.hpp"
#endif//_WIN32

namespace AN {


ImageView *ImageView::Alloc() {
#ifdef _WIN32
    return new WIN::ImageView();
#else
#error "not implement"
#endif//_WIN32
}


}
