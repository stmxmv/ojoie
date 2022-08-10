//
// Created by Aleudillonam on 8/8/2022.
//

#ifndef OJOIE_NODE2D_HPP
#define OJOIE_NODE2D_HPP

#include <ojoie/Core/Node.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {


class Node2D : public Node {
    typedef Node2D Self;
    typedef Node Super;


    Math::vec2 _position{};
protected:
    Math::vec2 r_position{};

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


    const Math::vec2 &getPosition() const {
        return _position;
    }

    void setPosition(const Math::vec2 &position) {
        _position = position;
        Dispatch::async(Dispatch::Render, [position, weakSelf = weak_from_this()] {
            auto _self = weakSelf.lock();
            if (_self) {
                Self *self = (Self *) _self.get();
                self->r_position = position;
            }
        });
    }


    const Math::vec2 &getPositionR() const {
        return r_position;
    }

};

}

#endif//OJOIE_NODE2D_HPP
