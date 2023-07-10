//
// Created by aojoie on 5/19/2023.
//

#include "Core/DragAndDrop.hpp"
#include "Core/Event.hpp"
#include "Core/Game.hpp"
#include "Core/private/win32/DragAndDropImpl.hpp"
#include "Core/private/win32/Window.hpp"
#include "Utility/win32/Unicode.hpp"
#include <ShlObj_core.h>

namespace AN {

bool DragAndDrop::init() {
    InitializeDragAndDrop();
    return true;
}

void DragAndDrop::deinit() {
    ShutdownDragAndDrop();
}

}

namespace AN::WIN {

class DragAndDrop : public AN::DragAndDrop {

    std::vector<std::string>	m_CurrentDragPaths;

public:

    bool fetchDataFromDrag(IDataObject* data);

    void setup() {
        _dragVisualMode = kDragOperationNone;
        m_AcceptedDrag = false;
    }

    void cleanup() {
        m_CurrentDragPaths.clear();
    }

    void setUpdating(bool val) { bIsUpdating = val; }

    virtual const std::vector<std::string> &getPaths() override {
        return m_CurrentDragPaths;
    }
};

static WIN::DragAndDrop gDragAndDrop;;

struct WindowsDropTargetImpl : public IWindowsDropTarget {

    virtual bool UnderstandsData(IDataObject *data) {
        return gDragAndDrop.fetchDataFromDrag(data);
    }

    virtual DWORD DragEntered(void *userData, DWORD keyState, const POINTL &pt) {
        return DragEvent(userData, keyState, pt, kDragEnter);
    }

    virtual DWORD DragUpdated(void *userData, DWORD keyState, const POINTL &pt) {
        gDragAndDrop.setUpdating(true);
        return DragEvent(userData, keyState, pt, kDragUpdated);
    }

    virtual void DragPerformed(void *userData, DWORD keyState, const POINTL &pt) {
        DragEvent(userData, keyState, pt, kDragPerform);
        DragExited(userData);
    }

    virtual void DragExited(void *userData) {
        gDragAndDrop.setUpdating(false);
        POINTL pt = {0, 0};
        DragEvent(userData, 0, pt, kDragExited);
    }

    DWORD DragEvent(void *userData, DWORD keyState, const POINTL &pt, EventType eventType) {
        WIN::Window *window = (WIN::Window *)userData;

        gDragAndDrop.setup();

        Event::Current().setTypeInternal(eventType);
        window->dragEvent(keyState, pt, eventType);

        GetGame().tick();

        if (eventType == kDragExited) {
            gDragAndDrop.cleanup();
        }

        switch (gDragAndDrop.getVisualMode()) {
            case kDragOperationCopy:
                return DROPEFFECT_COPY;
            case kDragOperationLink:
                return DROPEFFECT_LINK;
            case kDragOperationGeneric:
                return DROPEFFECT_COPY;
            case kDragOperationMove:
                return DROPEFFECT_MOVE;
            default:
                return DROPEFFECT_NONE;
        }
    }

};

static WindowsDropTargetImpl s_DropTargetImpl;


bool DragAndDrop::fetchDataFromDrag(IDataObject *data) {
    bool understands = false;

    // read paths data
    m_CurrentDragPaths.clear();
    FORMATETC fmtPaths = { (CLIPFORMAT) GetClipboardFormatPaths(), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    if (data->QueryGetData(&fmtPaths) == S_OK) {
        STGMEDIUM stgmed;
        if (data->GetData(&fmtPaths, &stgmed) == S_OK) {
            understands |= true;

            const UInt8 *ptr = reinterpret_cast<const UInt8 *>(GlobalLock(stgmed.hGlobal));
            if (ptr) {
                int count = *(const Int32 *) ptr;
                ptr += sizeof(Int32);
                m_CurrentDragPaths.reserve(count);
                for (int i = 0; i < count; ++i) {
                    int length = *(const Int32 *) ptr;
                    ptr += sizeof(Int32);
                    std::string path((const char *) ptr, length);
                    m_CurrentDragPaths.push_back(path);
                    ptr += length;
                }
                GlobalUnlock(stgmed.hGlobal);
            }
            ReleaseStgMedium(&stgmed);
        }
    }

    // read file drag data
    if (!understands) {
        FORMATETC fmtFiles = { (CLIPFORMAT) GetClipboardFormatFiles(), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (data->QueryGetData(&fmtFiles) == S_OK) {
            STGMEDIUM stgmed;
            if (data->GetData(&fmtFiles, &stgmed) == S_OK) {
                understands |= true;

                const DROPFILES *ptr = reinterpret_cast<const DROPFILES *>(GlobalLock(stgmed.hGlobal));
                if (ptr) {
                    if (ptr->fWide) {
                        int offset = ptr->pFiles;

                        while (((char *) ptr + offset)[0] != 0) {
                            std::string path;
                            wchar_t    *widePath = (wchar_t *) ((char *) ptr + offset);

                            m_CurrentDragPaths.push_back(WideToUtf8(widePath));
                            offset += wcslen(widePath) * sizeof(wchar_t) + 2;
                        }
                    } else {
                        // ansi path names are not handled by unity.
                        // this usually indicates some sort of drop source error (see bug 213936)
                    }

                    GlobalUnlock(stgmed.hGlobal);
                }
                ReleaseStgMedium(&stgmed);
            }
        }
    }

    return understands;
}


}

namespace AN {


void *DragAndDrop::registerWindowForDrop(Window *window) {
    IDropTarget* drop = nullptr;
    RegisterDropWindow(((WIN::Window *)window)->getHWND(), WIN::s_DropTargetImpl, window, &drop );
    ANAssert(drop != nullptr);
    return drop;
}

void DragAndDrop::unregisterWindowForDrop(Window *window, void *dropData) {
    ANAssert(dropData != nullptr);
    IDropTarget* drop = reinterpret_cast<IDropTarget*>( dropData );
    UnregisterDropWindow(((WIN::Window *)window)->getHWND(), drop);
}

DragAndDrop &GetDragAndDrop() {
    return WIN::gDragAndDrop;
}

}