#ifndef NANODBC_UNICODE_UTILS_H
#define NANODBC_UNICODE_UTILS_H

#include <nanodbc/nanodbc.h>

#include <stdexcept>
#include <string>
#include <type_traits>

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
#error Examples do not support the iODBC wide strings
#endif

// UTF-8 conversion for the test and example helpers.
//
// std::wstring_convert and the std::codecvt facets were deprecated in C++17 and removed in
// C++26, so the conversion is done here instead. The wide encoding follows the width of
// nanodbc::string::value_type: four bytes is UTF-32, two bytes is UTF-16.
namespace detail
{

inline void append_as_utf8(char32_t cp, std::string& out)
{
    if (cp < 0x80)
        out.push_back(static_cast<char>(cp));
    else if (cp < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

template <class T>
inline void append_as_wide(char32_t cp, std::basic_string<T>& out)
{
    if (sizeof(T) == 4 || cp < 0x10000)
    {
        out.push_back(static_cast<T>(cp));
    }
    else
    {
        cp -= 0x10000;
        out.push_back(static_cast<T>(0xD800 + (cp >> 10)));
        out.push_back(static_cast<T>(0xDC00 + (cp & 0x3FF)));
    }
}

inline char32_t next_utf8_code_point(char const*& beg, char const* end)
{
    auto const lead = static_cast<unsigned char>(*beg++);
    if (lead < 0x80)
        return lead;

    int extra;
    char32_t cp;
    if (lead >= 0xC2 && lead <= 0xDF)
    {
        extra = 1;
        cp = lead & 0x1F;
    }
    else if (lead >= 0xE0 && lead <= 0xEF)
    {
        extra = 2;
        cp = lead & 0x0F;
    }
    else if (lead >= 0xF0 && lead <= 0xF4)
    {
        extra = 3;
        cp = lead & 0x07;
    }
    else
        throw std::range_error("UTF-8 -> wide conversion error");

    if (end - beg < extra)
        throw std::range_error("UTF-8 -> wide conversion error");
    for (int i = 0; i < extra; ++i)
    {
        auto const byte = static_cast<unsigned char>(*beg++);
        if ((byte & 0xC0) != 0x80)
            throw std::range_error("UTF-8 -> wide conversion error");
        cp = (cp << 6) | (byte & 0x3F);
    }
    if ((extra == 1 && cp < 0x80) || (extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000) ||
        cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        throw std::range_error("UTF-8 -> wide conversion error");
    return cp;
}

template <class T>
inline char32_t next_wide_code_point(T const*& beg, T const* end)
{
    char32_t const unit =
        static_cast<char32_t>(static_cast<typename std::make_unsigned<T>::type>(*beg++));
    if (sizeof(T) == 4 || unit < 0xD800 || unit > 0xDFFF)
        return unit;
    if (unit >= 0xDC00 || beg == end)
        throw std::range_error("wide -> UTF-8 conversion error");
    char32_t const trail =
        static_cast<char32_t>(static_cast<typename std::make_unsigned<T>::type>(*beg));
    if (trail < 0xDC00 || trail > 0xDFFF)
        throw std::range_error("wide -> UTF-8 conversion error");
    ++beg;
    return 0x10000 + ((unit - 0xD800) << 10) + (trail - 0xDC00);
}

} // namespace detail

#ifdef NANODBC_ENABLE_UNICODE
inline nanodbc::string convert(std::string const& in)
{
    static_assert(
        sizeof(nanodbc::string::value_type) > 1,
        "NANODBC_ENABLE_UNICODE mode requires wide string");
    nanodbc::string out;
    auto const* beg = in.data();
    auto const* const end = beg + in.size();
    while (beg != end)
        detail::append_as_wide(detail::next_utf8_code_point(beg, end), out);
    return out;
}

inline std::string convert(nanodbc::string const& in)
{
    static_assert(sizeof(nanodbc::string::value_type) > 1, "string must be wide");
    std::string out;
    auto const* beg = in.data();
    auto const* const end = beg + in.size();
    while (beg != end)
        detail::append_as_utf8(detail::next_wide_code_point(beg, end), out);
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
