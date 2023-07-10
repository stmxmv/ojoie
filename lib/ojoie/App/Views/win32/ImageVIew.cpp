//
// Created by aojoie on 6/18/2023.
//

#include "App/Views/win32/ImageView.hpp"
#include "Render/TextureLoader.hpp"
#include "Utility/win32/Unicode.hpp"

extern HINSTANCE gHInstance;

namespace AN::WIN {

static void ConvertRGBAtoBGRA(unsigned char *imageData, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixelIndex = (y * width + x) * 4;

            unsigned char temp        = imageData[pixelIndex];    // Store red channel
            imageData[pixelIndex]     = imageData[pixelIndex + 2];// Assign blue channel to red
            imageData[pixelIndex + 2] = temp;                     // Assign stored red channel to blue
        }
    }
}

HBITMAP LoadPNGFromResource(HDC dc, const wchar_t *name, int& width, int& height) {
    HINSTANCE instance = gHInstance;
    HBITMAP   bitmap   = nullptr;
    width              = 0;
    height             = 0;
    HRSRC resource     = FindResourceW(instance, name, L"IMAGE");
    if (resource) {
        DWORD   size = SizeofResource(instance, resource);
        HGLOBAL glob = LoadResource(instance, resource);
        if (glob) {
            void *data = LockResource(glob);
            if (data) {
                LoadTextureResult result = TextureLoader::LoadTextureFromMemory((UInt8 *)data, size);

                if (!result.isValid() && result.getPixelFormat() != kPixelFormatRGBA8Unorm_sRGB)
                    return bitmap;

                width = result.getWidth();
                height = result.getHeight();

                BITMAPINFO bi;
                memset(&bi, 0, sizeof(bi));
                bi.bmiHeader.biSize        = sizeof(bi.bmiHeader);
                bi.bmiHeader.biWidth       = width;
                bi.bmiHeader.biHeight      = -height;
                bi.bmiHeader.biBitCount    = 32;
                bi.bmiHeader.biPlanes      = 1;
                bi.bmiHeader.biCompression = BI_RGB;
                unsigned char *bitmapData  = NULL;
                bitmap                     = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void **) &bitmapData, NULL, 0);
                if (bitmap) {
                    ConvertRGBAtoBGRA(result.getData(), result.getWidth(), result.getHeight());
                    memcpy(bitmapData, result.getData(), width * height * 4);
                }
            }
        }
    }
    return bitmap;
}

ImageView::ImageView() : imageWidth(), imageHeight(), hbmpSplash() {}

bool ImageView::init(const char *imageName) {

    HDC hdcScreen = GetDC(nullptr);

    hbmpSplash = LoadPNGFromResource(hdcScreen, Utf8ToWide(imageName).c_str(), imageWidth, imageHeight);

    if (!View::init({ .size = { (UInt32) imageWidth, (UInt32) imageHeight } })) {
        ReleaseDC(nullptr, hdcScreen);
        ::DeleteObject(hbmpSplash);
        return false;
    }

    ReleaseDC(nullptr, hdcScreen);

    InvalidateRect(getHWND(), nullptr, true);
    UpdateWindow(getHWND());
    return true;
}

ImageView::~ImageView() {
    if (hbmpSplash) {
        ::DeleteObject(hbmpSplash);
        hbmpSplash = nullptr;
    }
}

LRESULT ImageView::handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
        case WM_PAINT:
        {
            PAINTSTRUCT     ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            HDC hdcMem = CreateCompatibleDC(hdc);

            HGDIOBJ oldBitmap = SelectObject(hdcMem, hbmpSplash);

            // use the source image's alpha channel for blending
            BLENDFUNCTION blend       = { 0 };
            blend.BlendOp             = AC_SRC_OVER;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat         = AC_SRC_ALPHA;

            // paint the window (in the right location) with the alpha-blended bitmap
            SIZE sizeSplash = { imageWidth, imageHeight };

            POINT ptZero{};
            HDC hdcScreen = GetDC(nullptr);
            UpdateLayeredWindow(getHWND(), hdcScreen, nullptr, &sizeSplash, hdcMem, &ptZero, RGB(255, 0, 255), &blend, ULW_ALPHA);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);
            ReleaseDC(nullptr, hdcScreen);

            EndPaint(hWnd, &ps);
        }
            break;

        default:
            break;
    }
    return View::handleMessageInternal(hWnd, msg, wParam, lParam);
}


}