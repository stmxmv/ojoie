//
// Created by aojoie on 6/18/2023.
//

#pragma once

#include <ojoie/App/Views/View.hpp>

namespace AN {

class AN_API ImageView : virtual public View {

public:

    static ImageView *Alloc();

    virtual bool init(const char *imageName) = 0;


};



}
