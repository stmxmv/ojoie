//
//  Timer.hpp
//  WNITE
//
//  Created by Molybdenum on 11/27/21.
//

#ifndef Timer_hpp
#define Timer_hpp
#include <chrono>
namespace AN {


struct Timer {
    std::chrono::steady_clock::time_point last;

    float deltaTime;

    float elapsedTime;

    Timer() : last(std::chrono::steady_clock::now()), deltaTime(), elapsedTime() {}


    auto now() {
        return std::chrono::steady_clock::now();
    }

    float mark() {
        const auto old = last;
        last = std::chrono::steady_clock::now();
        const std::chrono::duration<float> frameTime = last - old;

        float frameTimeCount = frameTime.count();

        elapsedTime += frameTimeCount;

        deltaTime = frameTimeCount;

        return frameTimeCount;
    }

    float peek() const {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - last).count();
    }

};





}


#endif /* Timer_hpp */
