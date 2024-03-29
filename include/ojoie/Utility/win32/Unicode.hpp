//
// Created by aojoie on 5/6/2023.
//

#ifndef OJOIE_UNICODE_HPP
#define OJOIE_UNICODE_HPP

//#include <ojoie/Configuration/typedef.h>
#include <string>
#include <stringapiset.h>

// Implementation details for ConvertUTF8ToWideString/ConvertWideToUTF8String
namespace detail {

struct multi_byte_to_wide {
    int get_length(const char *utf8, std::size_t length) const {
        return ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(length), NULL, 0);
    }

    void convert(const char *utf8, std::size_t length, std::wstring &dest) const {
        // Convert the given narrow string to wchar_t format
        ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(length), &dest.front(),
                              static_cast<int>(dest.size()));
    }
};

struct wide_to_multi_byte {
    explicit wide_to_multi_byte(UINT codepage)
        : m_CodePage(codepage) {}

    int get_length(const wchar_t *wide, std::size_t length) const {
        return ::WideCharToMultiByte(m_CodePage, 0, wide, static_cast<int>(length), NULL, 0, NULL, NULL);
    }

    void convert(const wchar_t *wide, std::size_t length, std::string &dest) const {
        // Convert the given wide string to char format
        ::WideCharToMultiByte(m_CodePage, 0, wide, static_cast<int>(length), &dest.front(),
                              static_cast<int>(dest.size()), NULL, NULL);
    }

private:
    UINT m_CodePage;
};

// The reason for the length parameter is that we do not want to pass -1 to MultiByteToWideChar function.
// If cbMultiByte parameter is -1, the function processes the entire input string, including the terminating null character.
// Therefore, the resulting Unicode string has a terminating null character, and the length returned by the function
// includes this character.
// Since we output the converted string of characters to std::wstring directly,
// we do not want to embed null characters in the string.
template<typename SourceString, typename DestString, typename Converter>
inline void ChangeStringFormat(const SourceString &src, std::size_t length, DestString &dest, Converter func) {
    if (0u == length)// source string is empty
    {
        return dest.clear();
    }

    // required size
    const int nChars = func.get_length(src, length);
    if (nChars <= 0)// destination string is empty
    {
        return dest.clear();
    } else {
        // properly resize the buffer
        dest.resize(nChars);
        return func.convert(src, length, dest);
    }
}

}// namespace detail

inline void ConvertUTF8ToWideString(const char *utf8, std::wstring &wide) {
    return detail::ChangeStringFormat(utf8, strlen(utf8), wide, detail::multi_byte_to_wide());
}

inline void ConvertUTF8ToWideString(const std::string &utf8, std::wstring &wide) {
    return detail::ChangeStringFormat(utf8.data(), utf8.size(), wide, detail::multi_byte_to_wide());
}

inline void ConvertUTF8ToWideString(const std::string_view &utf8, std::wstring &wide) {
    return detail::ChangeStringFormat(utf8.data(), utf8.size(), wide, detail::multi_byte_to_wide());
}

inline void ConvertWideToUTF8String(const wchar_t *wide, std::string &utf8) {
    return detail::ChangeStringFormat(wide, wcslen(wide), utf8,
                                      detail::wide_to_multi_byte(CP_UTF8));
}

inline void ConvertWideToUTF8String(const std::wstring &wide, std::string &utf8) {
    return detail::ChangeStringFormat(wide.data(), wide.size(), utf8,
                                      detail::wide_to_multi_byte(CP_UTF8));
}

inline void ConvertWideToUTF8String(const std::wstring_view &wide, std::string &utf8) {
    return detail::ChangeStringFormat(wide.data(), wide.size(), utf8,
                                      detail::wide_to_multi_byte(CP_UTF8));
}


inline void ConvertToDefaultAnsi(const std::wstring &wide, std::string &ansi) {
    detail::ChangeStringFormat(wide.data(), wide.size(), ansi,
                               detail::wide_to_multi_byte(CP_ACP));
}

inline std::string ConvertToDefaultAnsi(const std::string &utf8) {
    std::wstring wide;
    ConvertUTF8ToWideString(utf8, wide);

    std::string bufAnsi;
    ConvertToDefaultAnsi(wide, bufAnsi);
    return bufAnsi;
}


// Implementation details for WideToUtf8/Utf8ToWide
namespace detail {

template<typename WideString>
inline std::string WideToUtf8Impl(const WideString &wide) {
    std::string result;
    ConvertWideToUTF8String(wide, result);
    return result;
}

template<typename NarrowString>
inline std::wstring Utf8ToWideImpl(const NarrowString &utf8) {
    std::wstring result;
    ConvertUTF8ToWideString(utf8, result);
    return result;
}

}// namespace detail

inline std::string WideToUtf8(const wchar_t *wide) {
    return detail::WideToUtf8Impl(wide);
}

inline std::string WideToUtf8(const std::wstring &wide) {
    return detail::WideToUtf8Impl(wide);
}

inline std::string WideToUtf8(const std::wstring_view &wide) {
    return detail::WideToUtf8Impl(wide);
}

inline std::wstring Utf8ToWide(const char *utf8) {
    return detail::Utf8ToWideImpl(utf8);
}

inline std::wstring Utf8ToWide(const std::string &utf8) {
    return detail::Utf8ToWideImpl(utf8);
}

inline std::wstring Utf8ToWide(const std::string_view &utf8) {
    return detail::Utf8ToWideImpl(utf8);
}

#endif//OJOIE_UNICODE_HPP
