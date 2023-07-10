//
// Created by aojoie on 6/18/2023.
//

#pragma once

#include <ojoie/App/Views/View.hpp>

#include <Windows.h>

namespace AN::WIN {

class View : virtual public AN::View {

    HWND hWnd;

public:

    static void RegisterWindowClass();
    static void UnregisterWindowClass();

    View();

    virtual bool init(const Rect &frame) override;

    virtual ~View() override;

    virtual Rect getFrame() override;

    HWND getHWND() const { return hWnd; }

    virtual LRESULT handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}
