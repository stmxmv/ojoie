//
// Created by aojoie on 4/22/2023.
//

#ifndef OJOIE_WIN32_WINDOW_HPP
#define OJOIE_WIN32_WINDOW_HPP

#include <ojoie/Render/Layer.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Event.hpp>
#include <Windows.h>

namespace AN::WIN {

class Window : public AN::Window {

    HWND hWnd;
    DWORD style;

    float _DPIScaleX, _DPIScaleY;

    bool bActiveInNonClientArea = false;

    WNDPROC _procCallback = nullptr;

    /// use to restore non-fullscreen
    RECT _oldRect;
    bool _oldZoom;
    bool bIsFullScreen = false;

    HACCEL _menuAccelerators = nullptr;

    void *dropData;

    bool bBridge;

    void retrieveDPIScale();
    void showCursor(bool show);
    void confineCursor(bool confine);

    std::unique_ptr<Layer> _layer;

public:

    static void RegisterWindowClass();
    static void UnregisterWindowClass();

    Window();

    void bridge(HWND hWnd);

    virtual bool init(const Rect &frame, bool wantsLayer) override;

    virtual ~Window() override;

    virtual void setTitle(const char *title) override;

    virtual void orderOut() override;

    virtual void orderFront() override;

    virtual void makeKey() override;

    virtual bool isVisible() override;

    virtual void setFrame(const Rect &frame) override;
    virtual Rect getFrame() override;
    virtual void center() override;
    virtual void setCursorState(CursorState state) override;
    virtual void setCursorShape(CursorShape shape) override;
    virtual void setCursorShape(const char *name) override;
    virtual Point       getCursorPosition() override;
    virtual void        setCursorPosition(const Point &point) override;
    virtual CursorShape getCursorShape() const override;

    virtual float getDPIScaleX() override { return _DPIScaleX; }
    virtual float getDPIScaleY() override { return _DPIScaleY; }

    virtual void setMenu(Menu *menu) override;

    virtual void setFullScreen(bool fullScreen) override;
    virtual bool isFullScreen() override { return bIsFullScreen; }

    virtual bool isZoomed() const override;
    virtual void zoom() override;

    virtual void setBorderLessStyle() override;

    virtual void setIMEInput(bool visible, UInt32 x, UInt32 y) override;

    virtual void addSubView(View *view) override;

    HWND getHWND() const { return hWnd; }

    HACCEL getMenuAccelerators() const { return _menuAccelerators; }

    WNDPROC setWNDPROCCallback(WNDPROC proc);

    LRESULT handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void dragEvent(DWORD keyState, const POINTL &pt, EventType eventType);

};

}

#endif//OJOIE_WIN32_WINDOW_HPP
