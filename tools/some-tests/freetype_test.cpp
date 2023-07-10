//
// Created by Aleudillonam on 7/29/2022.
//


#include <ojoie/Core/Log.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <Windows.h>
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>

#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl\client.h>

using Microsoft::WRL::ComPtr;

using std::cin, std::cout, std::endl;


bool loadSDF(int fontSize) {


    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        ANLog("ERROR::FREETYPE: Could not init FreeType Library");
        return false;
    }

    FT_Face face;

    if (FT_New_Face(ft, "C:\\Windows\\Fonts\\arial.ttf", 0, &face)) {
        ANLog("ERROR::FREETYPE: Failed to load font");
        return false;
    }


    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // we need to find the character that goes below the baseline by the biggest value
    int maxUnderBaseline = 0;

    for (unsigned char c = 0; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_DEFAULT)) {
            ANLog("ERROR::FREETYTPE: Failed to load Glyph");
            continue;
        }
        // get the glyph metrics
        const FT_Glyph_Metrics &glyphMetrics = face->glyph->metrics;

        // find the character that reaches below the baseline by the biggest value
        int glyphHang = (glyphMetrics.horiBearingY - glyphMetrics.height) >> 6;
        if (glyphHang < maxUnderBaseline) {
            maxUnderBaseline = glyphHang;
        }
    }

    int imageWidth = (fontSize+2)*16;
    int imageHeight = (fontSize+2)*8;

    std::vector<unsigned char> buffer(imageWidth * imageHeight * 4);

    std::vector<int> widths(128);

    FT_Error error;
    // draw all characters
    for (unsigned char i = 0; i < 128; ++i) {
        // load the glyph image into the slot
        error = FT_Load_Char(face, i, FT_LOAD_DEFAULT);
        if (error) {
            std::cout << "BitmapFontGenerator > failed to load glyph, error code: " << error << std::endl;
        }

        // convert to an anti-aliased bitmap
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            std::cout << "BitmapFontGenerator > failed to render glyph, error code: " << error << std::endl;
        }

        // save the character width
        widths[i] = face->glyph->metrics.width >> 6;

        // find the tile position where we have to draw the character
        int x = (i % 16) * (fontSize + 2);
        int y = (i / 16) * (fontSize + 2);
        x += 1;// 1 pixel padding from the left side of the tile
        y += (fontSize + 2) - face->glyph->bitmap_top + maxUnderBaseline - 1;

        // draw the character
        const FT_Bitmap &bitmap = face->glyph->bitmap;
        for (int xx = 0; xx < bitmap.width; ++xx) {
            for (int yy = 0; yy < bitmap.rows; ++yy) {
                unsigned char r                                      = bitmap.buffer[(yy * (bitmap.width) + xx)];
                buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 0] = r;
                buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 1] = r;
                buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 2] = r;
                buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 3] = 255;
            }
        }
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    stbi_write_bmp("font.bmp", imageWidth, imageHeight, 4, buffer.data());

    return true;
}

inline void ___assert_log(const char *expect, std::source_location location = std::source_location::current()) {
    ANLog("ANAssertion %s Failed in file %s (%u:%u) function: %s", expect, location.file_name(), location.line(), location.column(), location.function_name());
}

inline void ___assert_log(const char *expect,  const char *msg, std::source_location location = std::source_location::current()) {
    ANLog("ANAssertion %s Failed in file %s (%u:%u) function: %s message: %s", expect, location.file_name(), location.line(), location.column(), location.function_name(), msg);
}

#define an_assert(expect, ...) do { \
        if (!(expect)) {           \
            ___assert_log(#expect, ##__VA_ARGS__);\
        }\
    } while (0)



int main(int argc, const char * argv[]) {

    // Place your initialization logic here
    //    SimpleAudioManager audio;
    //    audio.Load("C:\\Users\\Aleudillonam\\Desktop\\02-The First Layer.flac");
    //
    //    audio.Play("C:\\Users\\Aleudillonam\\Desktop\\02-The First Layer.flac");
    //
    //    audio.Update();
    //
    //    std::this_thread::sleep_for(std::chrono::seconds(120));

    an_assert(false);

    an_assert(false, "some message");

    if (loadSDF(48)) {
        return 0;
    }

    return -1;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);


    ComPtr<IMMDeviceEnumerator> pEnumerator;

    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
        cout << "Create MMDeviceEnumerator fail" << endl;
    }


    ComPtr<IMMDeviceCollection> pDevices;
    pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL, &pDevices);

    UINT cDevices;
    pDevices->GetCount(&cDevices);

    for (UINT i = 0; i < cDevices; ++i) {
        ComPtr<IMMDevice> device;
        pDevices->Item(i, &device);

        /// get device id here
        //        LPWSTR deviceID;
        //        device->GetId(&deviceID);

        ComPtr<IPropertyStore> pProperties;
        device->OpenPropertyStore(STGM_READ, &pProperties);
        PROPVARIANT propVariant{};
        propVariant.vt = VT_LPWSTR;
        pProperties->GetValue(PKEY_Device_FriendlyName, &propVariant);

        std::string u8String;
        u8String.resize(WideCharToMultiByte(CP_UTF8, 0, propVariant.pwszVal, -1, nullptr, 0, nullptr, 0));
        WideCharToMultiByte(CP_UTF8, 0, propVariant.pwszVal, -1, u8String.data(), (int)u8String.size(), nullptr, 0);
        std::cout << u8String << endl;

    }


    CoUninitialize();

    return 0;
}