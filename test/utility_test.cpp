#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

// clang-format off
#define NANODBC_DISABLE_NANODBC_NAMESPACE_FOR_INTERNAL_TESTS
#include "nanodbc/nanodbc.cpp" // access private conversion routines
// clang-format on

#include <string>

TEST_CASE("convert", "[string]")
{
    std::string const u8 = (char const*)u8"Hello ツ World"; // FIXME: Hack! C++20 adds std::u8string
    std::u16string const u16 = u"Hello ツ World";
    std::u32string const u32 = U"Hello ツ World";
    std::wstring const w = L"Hello ツ World";

    SECTION("identity conversion")
    {
        SECTION("std::string to std::string (UTF-8)")
        {
            std::string out;
            convert(u8, out);
            REQUIRE(u8 == out);
        }

        SECTION("std::wstring to std::wstring (UTF-16 or UTF-32)")
        {
            std::wstring out;
            convert(w, out);
            REQUIRE(w == out);
        }

        SECTION("std::u16string to std::u16string")
        {
            std::u16string out;
            convert(u16, out);
            REQUIRE(u16 == out);
        }

        SECTION("std::u32string to std::u32string")
        {
            std::u32string out;
            convert(u32, out);
            REQUIRE(u32 == out);
        }
    }

    SECTION("widening conversion"){
#ifndef _MSC_VER
        SECTION("std::string to std::u16string"){std::u16string out;
    convert(u8, out);
    REQUIRE(u16 == out);
}
#else
        SECTION("std::string to std::wstring"){std::wstring out;
    convert(u8, out);
    REQUIRE(w == out);
}
#endif

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
SECTION("std::string to std::u32string")
{
    std::u32string out;
    convert(u8, out);
    REQUIRE(u32 == out);
}
#endif
}

SECTION("narrowing conversion")
{
#ifndef _MSC_VER
    SECTION("std::u16string to std::string")
    {
        std::string out;
        convert(u16, out);
        REQUIRE(u8 == out);
    }
#else
    SECTION("std::wstring to std::string")
    {
        std::string out;
        convert(w, out);
        REQUIRE(u8 == out);
    }
#endif

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
    SECTION("std::u32string to std::string")
    {
        std::string out;
        convert(u32, out);
        REQUIRE(u8 == out);
    }
#endif

    SECTION("SQLWCHAR via nanodbc::wide_char_t to std::string")
    {
#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
        static_assert(sizeof(WCHAR) == sizeof(char32_t), "WCHAR size is invalid");
        static_assert(sizeof(WCHAR) == sizeof(nanodbc::wide_char_t), "WCHAR size is invalid");

        std::string out;
        SQLWCHAR const* s = reinterpret_cast<WCHAR const*>(u32.data());
        auto const us = reinterpret_cast<nanodbc::wide_char_t const*>(
            s); // no-op or unsigned short to signed char16_t
        convert(us, u32.size(), out);
        REQUIRE(u8 == out);
#else
        static_assert(sizeof(WCHAR) == sizeof(char16_t), "WCHAR size is invalid");
        static_assert(sizeof(WCHAR) == sizeof(nanodbc::wide_char_t), "WCHAR size is invalid");

        std::string out;
        SQLWCHAR const* s = reinterpret_cast<WCHAR const*>(u16.data());
        auto const us = reinterpret_cast<nanodbc::wide_char_t const*>(
            s); // no-op or unsigned short to signed char16_t
        convert(us, u16.size(), out);
        REQUIRE(u8 == out);
#endif
    }
}
}
