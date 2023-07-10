//
// Created by aojoie on 5/19/2023.
//

#ifndef OJOIE_EVENT_HPP
#define OJOIE_EVENT_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {

enum EventType {
    kEventNone       = 0,
    kMouseDown       = 1,
    kMouseUp         = 2,
    kMouseMove       = 3,
    kMouseDrag       = 4,
    kKeyDown         = 5,
    kKeyUp           = 6,
    kScrollWheel     = 7,
    kRepaint         = 8,
    kLayout          = 9,
    kDragEnter       = 10,
    kDragUpdated     = 11,
    kDragPerform     = 12,
    kDragExited      = 15,
    kIgnore          = 11,
    kUsed            = 12,
    kValidateCommand = 13,
    kExecuteCommand  = 14,
    kContextClick    = 16,
    kMagnifyGesture  = 1000,
    kSwipeGesture    = 1001,
    kRotateGesture   = 1002
};

class AN_API Event {
    EventType _type;
public:

    static Event &Current();

    EventType getType() const { return _type; }

    void setTypeInternal(EventType type) { _type = type; }

};

inline Event &GetCurrentEvent() { return Event::Current(); }

}


#endif//OJOIE_EVENT_HPP
