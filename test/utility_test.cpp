#include "catch/catch_amalgamated.hpp"

// clang-format off
#define NANODBC_DISABLE_NANODBC_NAMESPACE_FOR_INTERNAL_TESTS
#include "nanodbc/nanodbc.cpp" // access private conversion routines
// clang-format on

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The UTF-8 codec replaced std::wstring_convert, so it is exercised directly here:
// round trips across every encoded length, the surrogate pairs UTF-16 needs for anything
// past the basic plane, and the malformed input it is expected to reject.
TEST_CASE("convert_utf8_round_trip", "[string][unicode]")
{
    // Built from explicit units rather than literals, so the test does not depend on the
    // encoding of this source file.
    struct sample
    {
        char const* name;
        std::string utf8;
        std::u16string utf16;
        std::u32string utf32;
    };

    std::vector<sample> const samples{
        {"ASCII", "A", u"A", U"A"},
        {"two byte (U+00E9)", "\xC3\xA9", u"\u00E9", U"\u00E9"},
        {"three byte (U+30C4)", "\xE3\x83\x84", u"\u30C4", U"\u30C4"},
        {"four byte (U+1F600)", "\xF0\x9F\x98\x80", {0xD83D, 0xDE00}, {0x1F600}},
        {"largest code point (U+10FFFF)", "\xF4\x8F\xBF\xBF", {0xDBFF, 0xDFFF}, {0x10FFFF}},
        {"embedded null is data",
         std::string("a\0b", 3),
         std::u16string(u"a\0b", 3),
         std::u32string(U"a\0b", 3)},
    };

    for (auto const& s : samples)
    {
        SECTION(s.name)
        {
            SECTION("utf-8 to wide and back")
            {
                nanodbc::wide_string wide;
                convert(s.utf8, wide);
#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
                REQUIRE(wide == nanodbc::wide_string(s.utf32.begin(), s.utf32.end()));
#else
                REQUIRE(wide == nanodbc::wide_string(s.utf16.begin(), s.utf16.end()));
#endif
                std::string utf8;
                convert(wide, utf8);
                REQUIRE(utf8 == s.utf8);
            }
        }
    }
}

TEST_CASE("convert_rejects_malformed_input", "[string][unicode]")
{
    SECTION("malformed utf-8")
    {
        auto const bad = GENERATE(
            std::string("\x80"),             // a continuation byte cannot lead
            std::string("\xC3"),             // truncated two byte sequence
            std::string("\xE3\x83"),         // truncated three byte sequence
            std::string("\xC0\xAF"),         // overlong encoding of '/'
            std::string("\xE0\x80\xAF"),     // overlong again, one byte longer
            std::string("\xED\xA0\x80"),     // a surrogate has no utf-8 encoding
            std::string("\xF5\x80\x80\x80"), // beyond U+10FFFF
            std::string("\xE3\x28\x84"));    // continuation byte is not one

        nanodbc::wide_string out;
        REQUIRE_THROWS_AS(convert(bad, out), std::range_error);
    }

#ifndef NANODBC_USE_IODBC_WIDE_STRINGS
    SECTION("malformed utf-16")
    {
        auto const bad = GENERATE(
            std::u16string(1, 0xD83D),     // high surrogate with nothing after it
            std::u16string(1, 0xDE00),     // low surrogate leading
            std::u16string{0xD83D, u'A'}); // high surrogate followed by a non surrogate

        std::string out;
        nanodbc::wide_string const wide(bad.begin(), bad.end());
        REQUIRE_THROWS_AS(convert(wide, out), std::range_error);
    }
#endif
}

TEST_CASE("convert", "[string]")
{
    // The cast is what carries the literal across the standards: it is char const* through
    // C++17 and char8_t const* from C++20.
    std::string const u8 = (char const*)u8"Hello ツ World";
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

    SECTION("widening conversion")
    {
#ifndef _MSC_VER
        SECTION("std::string to std::u16string")
        {
            std::u16string out;
            convert(u8, out);
            REQUIRE(u16 == out);
        }
#else
        SECTION("std::string to std::wstring")
        {
            std::wstring out;
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

// What a noexcept promises is not something a run can check. One that lies does not fail to
// compile and does not fail a test: it calls std::terminate, and only along the path it
// lied about, which is the path a suite is least likely to take. So the promises are made
// here, where the compiler checks them.

// Declared to throw nothing, and callers may rely on it.
static_assert(
    std::is_nothrow_default_constructible<nanodbc::result>::value,
    "result holds one shared_ptr, whose default constructor cannot throw");
static_assert(
    std::is_nothrow_default_constructible<nanodbc::result_iterator>::value,
    "result_iterator follows result");
static_assert(
    std::is_nothrow_default_constructible<nanodbc::date>::value &&
        std::is_nothrow_default_constructible<nanodbc::time>::value &&
        std::is_nothrow_default_constructible<nanodbc::timestamp>::value,
    "the temporal types are aggregates of integers");

// The handle accessors forward through a shared_ptr and reach no further.
static_assert(
    noexcept(std::declval<nanodbc::connection const&>().native_dbc_handle()) &&
        noexcept(std::declval<nanodbc::connection const&>().native_env_handle()),
    "the connection handles are read from a member");
static_assert(
    noexcept(std::declval<nanodbc::statement const&>().native_statement_handle()) &&
        noexcept(std::declval<nanodbc::result const&>().native_statement_handle()),
    "the statement handle is read from a member");
static_assert(
    noexcept(std::declval<nanodbc::result const&>().rowset_size()),
    "the rowset size is read from a member");

// And the other way about. These allocate, so they must not claim otherwise: saying
// noexcept over an allocation turns running out of memory into a call to terminate. If one
// of them stops allocating, say so here and take the noexcept with it.
static_assert(
    !std::is_nothrow_default_constructible<nanodbc::connection>::value,
    "connection allocates its implementation");
static_assert(
    !std::is_nothrow_default_constructible<nanodbc::statement>::value,
    "statement allocates its implementation");
static_assert(
    !std::is_nothrow_default_constructible<nanodbc::type_incompatible_error>::value &&
        !std::is_nothrow_default_constructible<nanodbc::null_access_error>::value &&
        !std::is_nothrow_default_constructible<nanodbc::index_range_error>::value,
    "the error types hand a literal to std::runtime_error, which may allocate");

// Which operations a type offers is decided by which ones it declares, and the deciding is
// done quietly: declaring a move constructor withdraws both assignments, and declaring a
// destructor withdraws the moves. Nothing about that fails to build until a caller reaches
// for the operation that went missing, so the shape of each public type is stated here.

// Copy and swap: constructible and assignable both ways, the assignment taking its
// argument by value and serving either. Each holds nothing but a shared_ptr, so none of the
// four can throw, which is what a container reads before deciding whether it may move on
// reallocation rather than copy. Adding a move assignment alongside it would make
// every assignment from an rvalue ambiguous, so the set stops at four on purpose.
#define NANODBC_ASSERT_COPY_AND_SWAP(T)                                                            \
    static_assert(std::is_copy_constructible<T>::value, #T " is copy constructible");              \
    static_assert(std::is_move_constructible<T>::value, #T " is move constructible");              \
    static_assert(std::is_copy_assignable<T>::value, #T " is copy assignable");                    \
    static_assert(std::is_move_assignable<T>::value, #T " is move assignable");                    \
    static_assert(std::is_nothrow_copy_constructible<T>::value, #T " copies a shared_ptr");        \
    static_assert(std::is_nothrow_move_constructible<T>::value, #T " moves a shared_ptr");         \
    static_assert(std::is_nothrow_copy_assignable<T>::value, #T " copy assigns a shared_ptr");     \
    static_assert(std::is_nothrow_move_assignable<T>::value, #T " move assigns a shared_ptr")

NANODBC_ASSERT_COPY_AND_SWAP(nanodbc::result);
NANODBC_ASSERT_COPY_AND_SWAP(nanodbc::statement);
NANODBC_ASSERT_COPY_AND_SWAP(nanodbc::connection);
NANODBC_ASSERT_COPY_AND_SWAP(nanodbc::transaction);
NANODBC_ASSERT_COPY_AND_SWAP(nanodbc::result_iterator);

#undef NANODBC_ASSERT_COPY_AND_SWAP

// table_valued_parameter is the odd one and not by design as far as anything says: it
// declares a move constructor, which withdrew both of its assignments. It can be built
// from another and never assigned from one. Stated so that restoring the assignments is a
// decision someone takes rather than a side effect of editing a constructor.
static_assert(
    std::is_copy_constructible<nanodbc::table_valued_parameter>::value &&
        std::is_move_constructible<nanodbc::table_valued_parameter>::value,
    "table_valued_parameter can be built from another");
static_assert(
    std::is_nothrow_copy_constructible<nanodbc::table_valued_parameter>::value &&
        std::is_nothrow_move_constructible<nanodbc::table_valued_parameter>::value,
    "and doing so copies a shared_ptr, which cannot throw");
static_assert(
    !std::is_copy_assignable<nanodbc::table_valued_parameter>::value &&
        !std::is_move_assignable<nanodbc::table_valued_parameter>::value,
    "table_valued_parameter has no assignment: its move constructor withdrew both");

// Catch is compiled without its own main(), so that the one large translation unit is
// built once for every test program rather than twice.
int main(int argc, char* argv[])
{
    Catch::Session session;
    session.configData().runOrder = Catch::TestRunOrder::Declared;
    return session.run(argc, argv);
}
