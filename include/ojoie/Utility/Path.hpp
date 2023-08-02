//
// Created by Aleudillonam on 7/4/2023.
//

#pragma once
#include <ojoie/Configuration/typedef.h>

#ifdef AN_WIN
#include <ojoie/Utility/win32/Path.hpp>
#endif

namespace AN {


inline constexpr char kPathNameSeparator = '/';

class Path {
    std::string _str;
public:

    explicit Path(std::string_view aPath) : _str(aPath) {}

    Path& removeLastComponent() {
        std::string::size_type p = _str.rfind(kPathNameSeparator);
        if (p == std::string::npos)
            return *this;
        else
            _str = std::string(_str.data(), p);
        return *this;
    }

    Path& appendComponent(std::string_view com) {
        std::string      res;
        std::string_view pathName(_str);
        res.reserve(pathName.size() + com.size() + 1);
        if (!pathName.empty() && !com.empty()) {
            if (pathName[pathName.size() - 1] == kPathNameSeparator) {
                if (com[0] == kPathNameSeparator) {
                    res += pathName;
                    res.append(com.begin() + 1, com.end());
                    _str = res;
                    return *this;
                } else {
                    res += pathName;
                    res += com;
                    _str = res;
                    return *this;
                }
            } else {
                if (com[0] == kPathNameSeparator) {
                    res += pathName;
                    res += com;
                    _str = res;
                    return *this;
                } else {
                    res += pathName;
                    res += kPathNameSeparator;
                    res += com;
                    _str = res;
                    return *this;
                }
            }
        } else if (pathName.empty())
            res = com;
        else
            res = pathName;
        _str = res;
        return *this;
    }

    std::string_view extension() const {
        std::string_view view(_str);
        auto length = view.length();
        for (size_t i=0;i< length;i++) {
            if (view[length - i - 1] == kPathNameSeparator)
                return {};
            if (view[length - i - 1] == '.')
                return view.substr(length - i);
        }
        return {};
    }

    std::string_view string_view() const { return _str; }
    std::string string() const { return _str; }
};


inline std::string ConvertPath(std::string_view path) {
#ifdef AN_WIN
    return ConvertWindowsPathName(Utf8ToWide(path));
#else
    return path;
#endif
}

}