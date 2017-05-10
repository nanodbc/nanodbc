#ifndef NANODBC_UNICODE_UTILS_H
#define NANODBC_UNICODE_UTILS_H

#include <nanodbc/nanodbc.h>

#include <codecvt>
#include <locale>
#include <string>

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
#error Examples do not support the iODBC wide strings
#endif

#ifdef NANODBC_ENABLE_UNICODE
inline nanodbc::string_type convert(std::string const& in)
{
    static_assert(
        sizeof(nanodbc::string_type::value_type) > 1,
        "NANODBC_ENABLE_UNICODE mode requires wide string_type");
    nanodbc::string_type out;
// Workaround for confirmed bug in VS2015 and VS2017 too
// See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
    using wide_char_t = nanodbc::string_type::value_type;
    auto s =
        std::wstring_convert<std::codecvt_utf8_utf16<wide_char_t>, wide_char_t>().from_bytes(in);
    auto p = reinterpret_cast<wide_char_t const*>(s.data());
    out.assign(p, p + s.size());
#else
    out = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(in);
#endif
    return out;
}

inline std::string convert(nanodbc::string_type const& in)
{
    static_assert(sizeof(nanodbc::string_type::value_type) > 1, "string_type must be wide");
    std::string out;
// Workaround for confirmed bug in VS2015 and VS2017 too
// See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
    using wide_char_t = nanodbc::string_type::value_type;
    std::wstring_convert<std::codecvt_utf8_utf16<wide_char_t>, wide_char_t> convert;
    auto p = reinterpret_cast<const wide_char_t*>(in.data());
    out = convert.to_bytes(p, p + in.size());
#else
    out = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(in);
#endif
    return out;
}
#else
inline nanodbc::string_type convert(std::string const& in)
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
inline std::string any_to_string<nanodbc::string_type>(nanodbc::string_type const& t)
{
    return convert(t);
}

#endif
