//
// Created by aojoie on 6/18/2023.
//

#pragma once

#include <App/Views/ImageView.hpp>
#include <App/Views/win32/View.hpp>

namespace AN::WIN {

class ImageView : public AN::ImageView, public WIN::View {

    int imageWidth, imageHeight;
    HBITMAP hbmpSplash;

public:

    ImageView();

    virtual bool init(const char *imageName) override;

    virtual ~ImageView() override;

    virtual LRESULT handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
};



}