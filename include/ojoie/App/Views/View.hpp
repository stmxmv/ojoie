//
// Created by aojoie on 6/18/2023.
//

#pragma once

#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Template/RC.hpp>

namespace AN {


class AN_API View : public RefCounted<View> {

public:

    static View *Alloc();

    virtual bool init(const Rect &frame) = 0;

    virtual Rect getFrame() = 0;

    virtual ~View() = default;

};



}