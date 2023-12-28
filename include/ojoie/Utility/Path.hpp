//
// Created by Aleudillonam on 7/4/2023.
//

#pragma once
#include <ojoie/Configuration/typedef.h>
#include <filesystem>

#ifdef AN_WIN
#include <ojoie/Utility/win32/Path.hpp>
#endif

namespace AN
{


inline constexpr char kPathNameSeparator = '/';

class Path
{
    std::string m_Str;

public:
    explicit Path(const std::string_view &aPath) : m_Str(aPath) {}

    Path &RemoveLastComponent()
    {
        std::string::size_type p = m_Str.rfind(kPathNameSeparator);
        if (p == std::string::npos)
            return *this;
        else
            m_Str = std::string(m_Str.data(), p);
        return *this;
    }

    Path &Append(const std::string_view &str)
    {
        m_Str.append(str);
        return *this;
    }

    Path &AppendComponent(const std::string_view &com)
    {
        std::string      res;
        std::string_view pathName(m_Str);
        res.reserve(pathName.size() + com.size() + 1);
        if (!pathName.empty() && !com.empty())
        {
            if (pathName[pathName.size() - 1] == kPathNameSeparator)
            {
                if (com[0] == kPathNameSeparator)
                {
                    res += pathName;
                    res.append(com.begin() + 1, com.end());
                    m_Str = res;
                    return *this;
                }
                else
                {
                    res += pathName;
                    res += com;
                    m_Str = res;
                    return *this;
                }
            }
            else
            {
                if (com[0] == kPathNameSeparator)
                {
                    res += pathName;
                    res += com;
                    m_Str = res;
                    return *this;
                }
                else
                {
                    res += pathName;
                    res += kPathNameSeparator;
                    res += com;
                    m_Str = res;
                    return *this;
                }
            }
        }
        else if (pathName.empty())
            res = com;
        else
            res = pathName;
        m_Str = res;
        return *this;
    }

    std::string_view GetLastComponent() const
    {
        std::string::size_type p = m_Str.rfind(kPathNameSeparator);
        if (p == std::string::npos)
        {
            return m_Str;
        }
        std::string_view view(m_Str);
        view.remove_prefix(p + 1);
        return view;
    }

    std::string_view GetLastComponentWithoutExtension() const
    {
        std::string::size_type p = m_Str.rfind(kPathNameSeparator);
        if (p == std::string::npos)
        {
            return m_Str;
        }
        std::string_view view(m_Str);
        view.remove_prefix(p + 1);

        auto ext = GetExtension();

        if (!ext.empty())
        {
            view.remove_suffix(ext.length() + 1);
        }
        return view;
    }

    std::string_view GetExtension() const
    {
        std::string_view view(m_Str);
        auto             length = view.length();
        for (size_t i = 0; i < length; ++i)
        {
            if (view[length - i - 1] == kPathNameSeparator)
                return {};
            if (view[length - i - 1] == '.')
                return view.substr(length - i);
        }
        return {};
    }

    void ReplaceExtension(const std::string_view &newExtension)
    {
        auto length = m_Str.length();
        for (size_t i = 0; i < length; ++i)
        {
            if (m_Str[length - i - 1] == kPathNameSeparator)
            {
                m_Str.append(".");
                m_Str.append(newExtension);
                return;
            }
            if (m_Str[length - i - 1] == '.')
            {
                m_Str.resize(length - i + newExtension.size());
                m_Str.replace(length - i, newExtension.size(), newExtension);
                return;
            }
        }

        m_Str.append(".");
        m_Str.append(newExtension);
    }

    bool Exists() const
    {
        std::filesystem::path stdPath(ToStdPath());
        return exists(stdPath);
    }

    bool IsDirectory() const
    {
        std::filesystem::path stdPath(ToStdPath());
        return is_directory(stdPath);
    }

    std::string_view      ToStringView() const { return m_Str; }
    std::string           ToString() const { return m_Str; }
    std::filesystem::path ToStdPath() const { return (const char8_t *) m_Str.c_str(); }
};


inline std::string ConvertPath(std::string_view path)
{
#ifdef AN_WIN
    return ConvertWindowsPathName(Utf8ToWide(path));
#else
    return path;
#endif
}

}// namespace AN