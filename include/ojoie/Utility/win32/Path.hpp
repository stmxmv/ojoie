//
// Created by Aleudillonam on 7/4/2023.
//

#pragma once

#include <ojoie/Utility/win32/Unicode.hpp>
#include <string_view>

namespace AN {


inline std::string ConvertWindowsPathName(const std::wstring_view& widePath) {
    std::string path(WideToUtf8(widePath));
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}


}
