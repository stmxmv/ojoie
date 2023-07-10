//
// Created by aojoie on 5/19/2023.
//

#ifndef OJOIE_DRAGANDDROP_HPP
#define OJOIE_DRAGANDDROP_HPP

#include <ojoie/Core/Window.hpp>

namespace AN {

enum DragVisualMode {
    kDragOperationNone = 0,
    kDragOperationCopy = 1,
    kDragOperationLink = 2,
    kDragOperationGeneric = 4,
    kDragOperationMove = 16,
    kDragOperationRejected = 32
};

class AN_API DragAndDrop {

protected:

    bool m_AcceptedDrag = false;
    bool bIsUpdating = false;
    DragVisualMode _dragVisualMode = kDragOperationNone;

public:

    bool init();

    void deinit();

    void* registerWindowForDrop(Window *window);

    void unregisterWindowForDrop(Window *window, void* dropData);

    void setVisualMode(DragVisualMode dragVisualMode) { _dragVisualMode = dragVisualMode; }

    virtual const std::vector<std::string> &getPaths() = 0;

    DragVisualMode getVisualMode() const { return _dragVisualMode; }

    bool isUpdating() const { return bIsUpdating; }
};

AN_API DragAndDrop &GetDragAndDrop();

}


#endif//OJOIE_DRAGANDDROP_HPP
