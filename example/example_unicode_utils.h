#ifndef NANODBC_UNICODE_UTILS_H
#define NANODBC_UNICODE_UTILS_H

#include <nanodbc/nanodbc.h>

#if defined(__GNUC__) && __GNUC__ < 5
#   include <cstdlib>
#else
#   include <codecvt>
#endif
#include <locale>
#include <string>

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
#error Examples do not support the iODBC wide strings
#endif

#ifdef NANODBC_ENABLE_UNICODE
inline nanodbc::string convert(std::string const& in)
{
    static_assert(
        sizeof(nanodbc::string::value_type) > 1,
        "NANODBC_ENABLE_UNICODE mode requires wide string");
    nanodbc::string out;
#if defined(__GNUC__) && __GNUC__ < 5
    std::vector<wchar_t> characters(in.length());
    for (size_t i=0; i<in.length(); i++)
        characters[i] = in[i];
    size_t size = wcstombs(nullptr, characters.data(), 0);
    if (size == std::string::npos)
        throw std::range_error("UTF-16 -> UTF8 conversion error");
    out.resize(size);
    wcstombs(&out[0], characters.data(), out.length());
#elif defined(_MSC_VER) && (_MSC_VER >= 1900)
    // Workaround for confirmed bug in VS2015 and VS2017 too
    // See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
    using wide_char_t = nanodbc::string::value_type;
    auto s =
        std::wstring_convert<std::codecvt_utf8_utf16<wide_char_t>, wide_char_t>().from_bytes(in);
    auto p = reinterpret_cast<wide_char_t const*>(s.data());
    out.assign(p, p + s.size());
#else
    out = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(in);
#endif
    return out;
}

inline std::string convert(nanodbc::string const& in)
{
    static_assert(sizeof(nanodbc::string::value_type) > 1, "string must be wide");
    std::string out;
#if defined(__GNUC__) && __GNUC__ < 5
    size_t size = mbstowcs(nullptr, in.data(), 0);
    if (size == std::string::npos)
        throw std::range_error("UTF-8 -> UTF16 conversion error");
    std::vector<wchar_t> characters(size);
    mbstowcs(&characters[0], in.data(), characters.size());
    out.resize(size);
    for (size_t i=0; i<in.length(); i++)
        out[i] = characters[i];
#elif defined(_MSC_VER) && (_MSC_VER >= 1900)
    // Workaround for confirmed bug in VS2015 and VS2017 too
    // See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
    using wide_char_t = nanodbc::string::value_type;
    std::wstring_convert<std::codecvt_utf8_utf16<wide_char_t>, wide_char_t> convert;
    auto p = reinterpret_cast<const wide_char_t*>(in.data());
    out = convert.to_bytes(p, p + in.size());
#else
    out = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(in);
#endif
    return out;
}
#else
inline nanodbc::string convert(std::string const& in)
{
    return in;
}
#endif

template <typename T>
inline std::string any_to_string(T const& t)
{
    return std::to_string(t);
}

template <>
inline std::string any_to_string<nanodbc::string>(nanodbc::string const& t)
{
    return convert(t);
}

#endif
