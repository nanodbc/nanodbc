/// \file nanodbc.cpp Implementation details.
#ifndef DOXYGEN

// ASCII art banners are helpful for code editors with a minimap display.
// Generated with http://patorjk.com/software/taag/#p=display&v=0&f=Colossal

#if defined(_MSC_VER)
#if _MSC_VER <= 1800
// silence spurious Visual C++ warnings
#pragma warning(disable : 4244) // warning about integer conversion issues.
#pragma warning(disable : 4312) // warning about 64-bit portability issues.
#endif
#pragma warning(disable : 4996) // warning about deprecated declaration
#endif

#include <nanodbc/nanodbc.h>

#include <algorithm>
#include <clocale>
#include <codecvt>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <type_traits>

#ifndef __clang__
#include <cstdint>
#endif

#ifdef NANODBC_HAS_STD_OPTIONAL
template <class T>
inline static void opt_reset(std::optional<T>& opt)
{
    opt.reset();
    return;
}
template <typename T, typename Enable = void>
struct is_optional : std::false_type
{
};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};
#endif

// User may redefine NANODBC_ASSERT macro in nanodbc/nanodbc.h
#ifndef NANODBC_ASSERT
#include <cassert>
#define NANODBC_ASSERT(expr) assert(expr)
#endif

#ifdef NANODBC_ENABLE_BOOST
#include <boost/locale/encoding_utf.hpp>
#elif defined(__GNUC__) && (__GNUC__ < 5)
#include <cwchar>
#else
#include <codecvt>
#endif

#ifdef __APPLE__
// silence spurious OS X deprecation warnings
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_6
#endif

#ifdef _WIN32
// needs to be included above sql.h for windows
#if !defined(__MINGW32__) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(_MSC_VER)
#ifndef NANODBC_ENABLE_UNICODE
// Disable unicode in sqlucode.h on Windows when NANODBC_ENABLE_UNICODE
// is not defined. This is required because unicode is enabled by
// default on many Windows systems.
#define SQL_NOUNICODEMAP
#endif
#endif

#include <sql.h>
#include <sqlext.h>

// Enable _variant_t support only for Visual C++ on Windows
#if defined(_MSC_VER)
#include <comutil.h>
#if defined(_DEBUG)
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif
#endif

// Driver specific SQL data type defines.
// Microsoft has -150 thru -199 reserved for Microsoft SQL Server Native Client driver usage.
// Originally, defined in sqlncli.h (old SQL Server Native Client driver)
// and msodbcsql.h (new Microsoft ODBC Driver for SQL Server)
// See https://github.com/nanodbc/nanodbc/issues/18
#ifndef SQL_SS_VARIANT
#define SQL_SS_VARIANT (-150)
#endif
#ifndef SQL_SS_XML
#define SQL_SS_XML (-152)
#endif
#ifndef SQL_SS_TABLE
#define SQL_SS_TABLE (-153)
#endif
#ifndef SQL_SS_TIME2
#define SQL_SS_TIME2 (-154)
#endif
#ifndef SQL_SS_TIMESTAMPOFFSET
#define SQL_SS_TIMESTAMPOFFSET (-155)
#endif
// Large CLR User-Defined Types (ODBC)
// https://msdn.microsoft.com/en-us/library/bb677316.aspx
// Essentially, UDT is a varbinary type with additional metadata.
// Memory layout: SQLCHAR *(unsigned char *)
// C data type:   SQL_C_BINARY
// Value:         SQL_BINARY (-2)
#ifndef SQL_SS_UDT
#define SQL_SS_UDT (-151) // from sqlncli.h
#endif

// Driver specific SQL constant defines for table-valued parameter
// From msodbcsql.h (new Microsoft ODBC Driver for SQL Server)
#ifndef SQL_SOPT_SS_BASE
#define SQL_SOPT_SS_BASE 1225
#endif
#ifndef SQL_SOPT_SS_PARAM_FOCUS
#define SQL_SOPT_SS_PARAM_FOCUS (SQL_SOPT_SS_BASE + 11)
#endif
#ifndef SQL_SOPT_SS_NAME_SCOPE
#define SQL_SOPT_SS_NAME_SCOPE (SQL_SOPT_SS_BASE + 12)
#endif
#ifndef SQL_SS_NAME_SCOPE_TABLE
#define SQL_SS_NAME_SCOPE_TABLE 0
#endif
#ifndef SQL_SS_NAME_SCOPE_TABLE_TYPE
#define SQL_SS_NAME_SCOPE_TABLE_TYPE 1
#endif
#ifndef SQL_SS_NAME_SCOPE_EXTENDED
#define SQL_SS_NAME_SCOPE_EXTENDED 2
#endif
#ifndef SQL_SS_NAME_SCOPE_SPARSE_COLUMN_SET
#define SQL_SS_NAME_SCOPE_SPARSE_COLUMN_SET 3
#endif
#ifndef SQL_SS_NAME_SCOPE_DEFAULT
#define SQL_SS_NAME_SCOPE_DEFAULT SQL_SS_NAME_SCOPE_TABLE
#endif
#ifndef SQL_CA_SS_BASE
#define SQL_CA_SS_BASE 1200
#endif
#ifndef SQL_CA_SS_TYPE_NAME
#define SQL_CA_SS_TYPE_NAME (SQL_CA_SS_BASE + 27)
#endif

// SQL_SS_LENGTH_UNLIMITED is used to describe the max length of
// VARCHAR(max), VARBINARY(max), NVARCHAR(max), and XML columns
#ifndef SQL_SS_LENGTH_UNLIMITED
#define SQL_SS_LENGTH_UNLIMITED (0)
#endif

// Max length of DBVARBINARY and DBVARCHAR, etc. +1 for zero byte
// MSDN: Large value data types are those that exceed the maximum row size of 8 KB
#define SQLSERVER_DBMAXCHAR (8000 + 1)

// Default to ODBC version defined by NANODBC_ODBC_VERSION if provided.
#ifndef NANODBC_ODBC_VERSION
#ifdef SQL_OV_ODBC3_80
// Otherwise, use ODBC v3.8 if it's available...
#define NANODBC_ODBC_VERSION SQL_OV_ODBC3_80
#else
// or fallback to ODBC v3.x.
#define NANODBC_ODBC_VERSION SQL_OV_ODBC3
#endif
#endif

// clang-format off
// 888     888          d8b                       888
// 888     888          Y8P                       888
// 888     888                                    888
// 888     888 88888b.  888  .d8888b .d88b.   .d88888  .d88b.
// 888     888 888 "88b 888 d88P"   d88""88b d88" 888 d8P  Y8b
// 888     888 888  888 888 888     888  888 888  888 88888888
// Y88b. .d88P 888  888 888 Y88b.   Y88..88P Y88b 888 Y8b.
//  "Y88888P"  888  888 888  "Y8888P "Y88P"   "Y88888  "Y8888
// MARK: Unicode -
// clang-format on

// Import string types defined in header file, so we don't have to type nanodbc:: everywhere
using nanodbc::wide_char_t;
using nanodbc::wide_string;

#ifdef NANODBC_ENABLE_UNICODE
#define NANODBC_FUNC(f) f##W
#define NANODBC_SQLCHAR SQLWCHAR
#else
#define NANODBC_FUNC(f) f
#define NANODBC_SQLCHAR SQLCHAR
#endif

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
#define NANODBC_CODECVT_TYPE std::codecvt_utf8
#else
#ifdef _MSC_VER
#define NANODBC_CODECVT_TYPE std::codecvt_utf8_utf16
#else
#define NANODBC_CODECVT_TYPE std::codecvt_utf8_utf16
#endif
#endif

#if defined(NANODBC_OVERALLOCATE_CHAR)
// If enabled, when auto-binding buffers to N/VAR/CHAR
// columns, overallocate assuming each code-point
// takes up MAX_CODE_POINT_SIZE bytes
#define MAX_CODE_POINT_SIZE 4
#define NBYTES(nchars, chartype) (nchars + 1) * MAX_CODE_POINT_SIZE
#else
#define NBYTES(nchars, chartype) (nchars + 1) * sizeof(chartype)
#endif

// clang-format off
//  .d88888b.  8888888b.  888888b.    .d8888b.       888b     d888
// d88P" "Y88b 888  "Y88b 888  "88b  d88P  Y88b      8888b   d8888
// 888     888 888    888 888  .88P  888    888      88888b.d88888
// 888     888 888    888 8888888K.  888             888Y88888P888  8888b.   .d8888b 888d888 .d88b.  .d8888b
// 888     888 888    888 888  "Y88b 888             888 Y888P 888     "88b d88P"    888P"  d88""88b 88K
// 888     888 888    888 888    888 888    888      888  Y8P  888 .d888888 888      888    888  888 "Y8888b.
// Y88b. .d88P 888  .d88P 888   d88P Y88b  d88P      888   "   888 888  888 Y88b.    888    Y88..88P      X88
//  "Y88888P"  8888888P"  8888888P"   "Y8888P"       888       888 "Y888888  "Y8888P 888     "Y88P"   88888P'
// MARK: ODBC Macros -
// clang-format on

#define NANODBC_STRINGIZE_I(text) #text
#define NANODBC_STRINGIZE(text) NANODBC_STRINGIZE_I(text)

// By making all calls to ODBC functions through this macro, we can easily get
// runtime debugging information of which ODBC functions are being called,
// in what order, and with what parameters by defining NANODBC_ODBC_API_DEBUG.
#ifdef NANODBC_ODBC_API_DEBUG
#include <iostream>
#define NANODBC_CALL_RC(FUNC, RC, ...)                                                             \
    do                                                                                             \
    {                                                                                              \
        std::cerr << __FILE__                                                                      \
            ":" NANODBC_STRINGIZE(__LINE__) " " NANODBC_STRINGIZE(FUNC) "(" #__VA_ARGS__ ")"       \
                  << std::endl;                                                                    \
        RC = FUNC(__VA_ARGS__);                                                                    \
    } while (false) /**/
#define NANODBC_CALL(FUNC, ...)                                                                    \
    do                                                                                             \
    {                                                                                              \
        std::cerr << __FILE__                                                                      \
            ":" NANODBC_STRINGIZE(__LINE__) " " NANODBC_STRINGIZE(FUNC) "(" #__VA_ARGS__ ")"       \
                  << std::endl;                                                                    \
        FUNC(__VA_ARGS__);                                                                         \
    } while (false) /**/
#else
#define NANODBC_CALL_RC(FUNC, RC, ...) RC = FUNC(__VA_ARGS__)
#define NANODBC_CALL(FUNC, ...) FUNC(__VA_ARGS__)
#endif

// clang-format off
// 8888888888                                      888    888                        888 888 d8b
// 888                                             888    888                        888 888 Y8P
// 888                                             888    888                        888 888
// 8888888    888d888 888d888 .d88b.  888d888      8888888888  8888b.  88888b.   .d88888 888 888 88888b.   .d88b.
// 888        888P"   888P"  d88""88b 888P"        888    888     "88b 888 "88b d88" 888 888 888 888 "88b d88P"88b
// 888        888     888    888  888 888          888    888 .d888888 888  888 888  888 888 888 888  888 888  888
// 888        888     888    Y88..88P 888          888    888 888  888 888  888 Y88b 888 888 888 888  888 Y88b 888
// 8888888888 888     888     "Y88P"  888          888    888 "Y888888 888  888  "Y88888 888 888 888  888  "Y88888
//                                                                                                             888
//                                                                                                        Y8b d88P
//                                                                                                         "Y88P"
// MARK: Error Handling -
// clang-format on

namespace
{
#ifdef NANODBC_ODBC_API_DEBUG
inline std::string return_code(RETCODE rc)
{
    switch (rc)
    {
    case SQL_SUCCESS:
        return "SQL_SUCCESS";
    case SQL_SUCCESS_WITH_INFO:
        return "SQL_SUCCESS_WITH_INFO";
    case SQL_ERROR:
        return "SQL_ERROR";
    case SQL_INVALID_HANDLE:
        return "SQL_INVALID_HANDLE";
    case SQL_NO_DATA:
        return "SQL_NO_DATA";
    case SQL_NEED_DATA:
        return "SQL_NEED_DATA";
    case SQL_STILL_EXECUTING:
        return "SQL_STILL_EXECUTING";
    }
    NANODBC_ASSERT(0);
    return "unknown"; // should never make it here
}
#endif

// Easy way to check if a return code signifies success.
inline bool success(RETCODE rc)
{
#ifdef NANODBC_ODBC_API_DEBUG
    std::cerr << "<-- rc: " << return_code(rc) << " | " << std::endl;
#endif
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

#if __cpp_lib_nonmember_container_access >= 201411 || _MSC_VER
using std::size;
#else
template <class T, std::size_t N>
constexpr std::size_t size(const T (&array)[N]) noexcept
{
    return N;
}
#endif

template <std::size_t N>
inline std::size_t size(NANODBC_SQLCHAR const (&array)[N]) noexcept
{
    auto const n = std::char_traits<NANODBC_SQLCHAR>::length(array);
    NANODBC_ASSERT(n < N);
    return n < N ? n : N - 1;
}

template <class T>
inline void convert(T const* beg, size_t n, std::basic_string<T>& out)
{
    out.assign(beg, n);
}

inline void convert(wide_char_t const* beg, size_t n, std::string& out)
{
#ifdef NANODBC_ENABLE_BOOST
    using boost::locale::conv::utf_to_utf;
    out = utf_to_utf<char>(beg, beg + n);
#elif defined(_MSC_VER) && (_MSC_VER == 1900)
    // Workaround for confirmed bug in VS2015. See:
    // https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
    // https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error
    // Why static? http://stackoverflow.com/questions/26196686/utf8-utf16-codecvt-poor-performance
    static thread_local std::wstring_convert<NANODBC_CODECVT_TYPE<unsigned short>, unsigned short>
        converter;
    out = converter.to_bytes(
        reinterpret_cast<unsigned short const*>(beg),
        reinterpret_cast<unsigned short const*>(beg + n));
#else
    static thread_local std::wstring_convert<NANODBC_CODECVT_TYPE<wide_char_t>, wide_char_t>
        converter;
    out = converter.to_bytes(beg, beg + n);
#endif
}

inline void convert(char const* beg, size_t n, wide_string& out)
{
#ifdef NANODBC_ENABLE_BOOST
    using boost::locale::conv::utf_to_utf;
    out = utf_to_utf<wide_char_t>(beg, beg + n);
#elif defined(_MSC_VER) && (_MSC_VER == 1900)
    // Workaround for confirmed bug in VS2015. See:
    // https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
    // https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error
    // Why static? http://stackoverflow.com/questions/26196686/utf8-utf16-codecvt-poor-performance
    static thread_local std::wstring_convert<NANODBC_CODECVT_TYPE<unsigned short>, unsigned short>
        converter;
    auto s = converter.from_bytes(beg, beg + n);
    auto p = reinterpret_cast<wide_char_t const*>(s.data());
    out.assign(p, p + s.size());
#else
    static thread_local std::wstring_convert<NANODBC_CODECVT_TYPE<wide_char_t>, wide_char_t>
        converter;
    out = converter.from_bytes(beg, beg + n);
#endif
}

template <class T>
inline void convert(char const* beg, std::basic_string<T>& out)
{
    convert(beg, std::strlen(beg), out);
}

template <class T>
inline void convert(wchar_t const* beg, std::basic_string<T>& out)
{
    convert(beg, std::wcslen(beg), out);
}

template <class T>
inline void convert(std::basic_string<T>&& in, std::basic_string<T>& out)
{
    out.assign(in);
}

template <class T, class U>
inline void convert(std::basic_string<T> const& in, std::basic_string<U>& out)
{
    convert(in.data(), in.size(), out);
}

// Attempts to get the most recent ODBC error as a string.
// Always returns std::string, even in unicode mode.
inline std::string
recent_error(SQLHANDLE handle, SQLSMALLINT handle_type, long& native, std::string& state)
{
    nanodbc::string result;
    std::string rvalue;
    std::vector<NANODBC_SQLCHAR> sql_message(SQL_MAX_MESSAGE_LENGTH);
    sql_message[0] = '\0';

    SQLINTEGER i = 1;
    SQLINTEGER native_error = 0;
    SQLSMALLINT total_bytes = 0;
    NANODBC_SQLCHAR sql_state[6] = {0};
    RETCODE rc;

    do
    {
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetDiagRec),
            rc,
            handle_type,
            handle,
            (SQLSMALLINT)i,
            sql_state,
            &native_error,
            nullptr,
            0,
            &total_bytes);

        if (success(rc) && total_bytes > 0)
            sql_message.resize(static_cast<std::size_t>(total_bytes) + 1);

        if (rc == SQL_NO_DATA)
            break;

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetDiagRec),
            rc,
            handle_type,
            handle,
            (SQLSMALLINT)i,
            sql_state,
            &native_error,
            sql_message.data(),
            (SQLSMALLINT)sql_message.size(),
            &total_bytes);

        if (!success(rc))
        {
            convert(std::move(result), rvalue);
            return rvalue;
        }

        if (!result.empty())
            result += ' ';

        result += nanodbc::string(sql_message.begin(), sql_message.end());
        i++;

// NOTE: unixODBC using PostgreSQL and SQLite drivers crash if you call SQLGetDiagRec()
// more than once. So as a (terrible but the best possible) workaround just exit
// this loop early on non-Windows systems.
#ifndef _MSC_VER
        break;
#endif
    } while (rc != SQL_NO_DATA);

    convert(std::move(result), rvalue);
    if (size(sql_state) > 0)
    {
        state.clear();
        state.reserve(size(sql_state));
        for (std::size_t idx = 0; idx != size(sql_state); ++idx)
        {
            state.push_back(static_cast<char>(sql_state[idx]));
        }
    }

    native = native_error;
    std::string status = state;
    status += ": ";
    status += rvalue;

    // some drivers insert \0 into error messages for unknown reasons
    using std::replace;
    replace(status.begin(), status.end(), '\0', ' ');

    return status;
}

} // namespace

#ifndef NANODBC_DISABLE_NANODBC_NAMESPACE_FOR_INTERNAL_TESTS
namespace nanodbc
{

type_incompatible_error::type_incompatible_error()
    : std::runtime_error("type incompatible")
{
}

const char* type_incompatible_error::what() const noexcept
{
    return std::runtime_error::what();
}

null_access_error::null_access_error()
    : std::runtime_error("null access")
{
}

const char* null_access_error::what() const noexcept
{
    return std::runtime_error::what();
}

index_range_error::index_range_error()
    : std::runtime_error("index out of range")
{
}

const char* index_range_error::what() const noexcept
{
    return std::runtime_error::what();
}

programming_error::programming_error(std::string const& info)
    : std::runtime_error(info.c_str())
{
}

const char* programming_error::what() const noexcept
{
    return std::runtime_error::what();
}

database_error::database_error(SQLHANDLE handle, short handle_type, std::string const& info)
    : std::runtime_error(info)
    , native_error(0)
    , sql_state("00000")
{
    message = std::string(std::runtime_error::what()) +
              recent_error(handle, handle_type, native_error, sql_state);
}

const char* database_error::what() const noexcept
{
    return message.c_str();
}

long database_error::native() const noexcept
{
    return native_error;
}

std::string const& database_error::state() const noexcept
{
    return sql_state;
}

} // namespace nanodbc

// Throwing exceptions using NANODBC_THROW_DATABASE_ERROR enables file name
// and line numbers to be inserted into the error message. Useful for debugging.
#ifdef NANODBC_THROW_NO_SOURCE_LOCATION
#define NANODBC_THROW_DATABASE_ERROR(handle, handle_type)                                          \
    throw nanodbc::database_error(handle, handle_type, "ODBC database error: ") /**/
#else
#define NANODBC_THROW_DATABASE_ERROR(handle, handle_type)                                          \
    throw nanodbc::database_error(                                                                 \
        handle, handle_type, __FILE__ ":" NANODBC_STRINGIZE(__LINE__) ": ") /**/
#endif

// clang-format off
// 8888888b.           888             d8b 888
// 888  "Y88b          888             Y8P 888
// 888    888          888                 888
// 888    888  .d88b.  888888  8888b.  888 888 .d8888b
// 888    888 d8P  Y8b 888        "88b 888 888 88K
// 888    888 88888888 888    .d888888 888 888 "Y8888b.
// 888  .d88P Y8b.     Y88b.  888  888 888 888      X88
// 8888888P"   "Y8888   "Y888 "Y888888 888 888  88888P'
// MARK: Details -
// clang-format on

#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_STMT_EVENT) &&                       \
    defined(SQL_API_SQLCOMPLETEASYNC)
#define NANODBC_DO_ASYNC_IMPL
#endif

namespace
{

using namespace std; // if int64_t is in std namespace (in c++11)

template <typename T>
using is_integral8 = std::integral_constant<
    bool,
    std::is_integral<T>::value && sizeof(T) == 1 && !std::is_same<T, char>::value>;

template <typename T>
using is_integral16 = std::integral_constant<
    bool,
    std::is_integral<T>::value && sizeof(T) == 2 && !std::is_same<T, wchar_t>::value>;

template <typename T>
using is_integral32 = std::integral_constant<
    bool,
    std::is_integral<T>::value && sizeof(T) == 4 && !std::is_same<T, wchar_t>::value>;

template <typename T>
using is_integral64 = std::integral_constant<bool, std::is_integral<T>::value && sizeof(T) == 8>;

// A utility for calculating the ctype from the given type T.
// I essentially create a lookup table based on the MSDN ODBC documentation.
// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms714556(v=vs.85).aspx
template <class T, typename Enable = void>
struct sql_ctype
{
};

template <>
struct sql_ctype<uint8_t>
{
    static const SQLSMALLINT value = SQL_C_BINARY;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral16<T>::value && std::is_signed<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_SSHORT;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral16<T>::value && std::is_unsigned<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_USHORT;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral32<T>::value && std::is_signed<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_SLONG;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral32<T>::value && std::is_unsigned<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_ULONG;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral64<T>::value && std::is_signed<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_SBIGINT;
};

template <typename T>
struct sql_ctype<
    T,
    typename std::enable_if<is_integral64<T>::value && std::is_unsigned<T>::value>::type>
{
    static const SQLSMALLINT value = SQL_C_UBIGINT;
};

template <>
struct sql_ctype<float>
{
    static const SQLSMALLINT value = SQL_C_FLOAT;
};

template <>
struct sql_ctype<double>
{
    static const SQLSMALLINT value = SQL_C_DOUBLE;
};

template <>
struct sql_ctype<wide_string::value_type>
{
    static const SQLSMALLINT value = SQL_C_WCHAR;
};

template <>
struct sql_ctype<wide_string>
{
    static const SQLSMALLINT value = SQL_C_WCHAR;
};

template <>
struct sql_ctype<std::string::value_type>
{
    static const SQLSMALLINT value = SQL_C_CHAR;
};

template <>
struct sql_ctype<std::string>
{
    static const SQLSMALLINT value = SQL_C_CHAR;
};

template <>
struct sql_ctype<nanodbc::date>
{
    static const SQLSMALLINT value = SQL_C_DATE;
};

template <>
struct sql_ctype<nanodbc::time>
{
    static const SQLSMALLINT value = SQL_C_TIME;
};

template <>
struct sql_ctype<nanodbc::timestamp>
{
    static const SQLSMALLINT value = SQL_C_TIMESTAMP;
};

// Encapsulates resources needed for column binding.
class bound_column
{
public:
    bound_column(bound_column const& rhs) = delete;
    bound_column& operator=(bound_column const& rhs) = delete;

    bound_column()
        : name_()
        , column_(0)
        , sqltype_(0)
        , sqlsize_(0)
        , scale_(0)
        , ctype_(0)
        , clen_(0)
        , blob_(false)
        , cbdata_(nullptr)
        , pdata_(nullptr)
        , bound_(false)
    {
    }

    ~bound_column() noexcept
    {
        delete[] cbdata_;
        delete[] pdata_;
    }

public:
    nanodbc::string name_;
    short column_;
    SQLSMALLINT sqltype_;
    SQLULEN sqlsize_;
    SQLSMALLINT scale_;
    SQLSMALLINT ctype_;
    SQLULEN clen_;
    bool blob_;
    nanodbc::null_type* cbdata_;
    char* pdata_;
    bool bound_;
};

// Encapsulates properties of statement parameter.
// Parameter corresponds to parameter marker associated with a prepared SQL statement.
struct bound_parameter
{
    bound_parameter() = default;

    SQLULEN size_ = 0;       // SQL data size of column or expression inbytes or characters
    SQLUSMALLINT index_ = 0; // Zero-based index of parameter marker
    SQLSMALLINT iotype_ = 0; // Input/Output type of parameter
    SQLSMALLINT type_ = 0;   // SQL data type of parameter
    SQLSMALLINT scale_ = 0;  // decimal digits of column or expression
};

// Encapsulates properties of buffer with data values bound to statement parameter.
template <typename T>
struct bound_buffer
{
    bound_buffer() = default;
    bound_buffer(T const* values, std::size_t size, std::size_t value_size = 0)
        : values_(values)
        , size_(size)
        , value_size_(value_size)
    {
    }

    T const* values_ = nullptr;  // Pointer to buffer for parameter's data
    std::size_t size_ = 0;       // Number of values (1 or length of array)
    std::size_t value_size_ = 0; // Size of single value (max size). Zero, if ignored.
};

inline void deallocate_handle(SQLHANDLE& handle, short handle_type)
{
    if (!handle)
        return;

    RETCODE rc;
    NANODBC_CALL_RC(SQLFreeHandle, rc, handle_type, handle);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(handle, handle_type);
    handle = nullptr;
}

inline void allocate_env_handle(SQLHENV& env)
{
    if (env)
        return;

    RETCODE rc;
    NANODBC_CALL_RC(SQLAllocHandle, rc, SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(env, SQL_HANDLE_ENV);

    try
    {
        NANODBC_CALL_RC(
            SQLSetEnvAttr,
            rc,
            env,
            SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER)NANODBC_ODBC_VERSION,
            SQL_IS_UINTEGER);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(env, SQL_HANDLE_ENV);
    }
    catch (...)
    {
        deallocate_handle(env, SQL_HANDLE_ENV);
        throw;
    }
}

inline void allocate_dbc_handle(SQLHDBC& conn, SQLHENV env)
{
    NANODBC_ASSERT(env);
    if (conn)
        return;

    try
    {
        RETCODE rc;
        NANODBC_CALL_RC(SQLAllocHandle, rc, SQL_HANDLE_DBC, env, &conn);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(env, SQL_HANDLE_ENV);
    }
    catch (...)
    {
        deallocate_handle(conn, SQL_HANDLE_DBC);
        throw;
    }
}

} // namespace

// nanodbc::attribute
#ifdef NANODBC_HAS_STD_VARIANT
namespace nanodbc
{
attribute::attribute(
    long const& attribute,
    long const& string_length,
    attribute::variant const& resource) noexcept
    : attribute_(attribute)
    , string_length_(string_length)
    , resource_(resource)
    , value_ptr_(nullptr)
{
    this->extractValuePtr();
}
attribute::attribute(attribute const& other) noexcept
    : attribute_(other.attribute_)
    , string_length_(other.string_length_)
    , resource_(other.resource_)
    , value_ptr_(nullptr)
{
    this->extractValuePtr();
}
void attribute::extractValuePtr()
{
    std::visit(
        [this](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (
                std::is_same_v<T, string> || std::is_same_v<T, std::string> ||
                std::is_same_v<T, std::vector<uint8_t>>)
            {
                this->value_ptr_ = (void*)&arg[0];
            }
            else if constexpr (
                std::is_same_v<T, std::intptr_t> || std::is_same_v<T, std::uintptr_t>)
            {
                this->value_ptr_ = (void*)(arg);
            }
        },
        this->resource_);
}
} // namespace nanodbc
#endif

// clang-format off
//  .d8888b.                                               888    d8b                             8888888                        888
// d88P  Y88b                                              888    Y8P                               888                          888
// 888    888                                              888                                      888                          888
// 888         .d88b.  88888b.  88888b.   .d88b.   .d8888b 888888 888  .d88b.  88888b.              888   88888b.d88b.  88888b.  888
// 888        d88""88b 888 "88b 888 "88b d8P  Y8b d88P"    888    888 d88""88b 888 "88b             888   888 "888 "88b 888 "88b 888
// 888    888 888  888 888  888 888  888 88888888 888      888    888 888  888 888  888             888   888  888  888 888  888 888
// Y88b  d88P Y88..88P 888  888 888  888 Y8b.     Y88b.    Y88b.  888 Y88..88P 888  888             888   888  888  888 888 d88P 888
//  "Y8888P"   "Y88P"  888  888 888  888  "Y8888   "Y8888P  "Y888 888  "Y88P"  888  888           8888888 888  888  888 88888P"  888
//                                                                                                                      888
//                                                                                                                      888
//                                                                                                                      888
// MARK: Connection Impl -
// clang-format on

namespace nanodbc
{

class connection::connection_impl
{
public:
    connection_impl(connection_impl const&) = delete;
    connection_impl& operator=(connection_impl const&) = delete;

    connection_impl()
        : env_(nullptr)
        , dbc_(nullptr)
        , connected_(false)
        , transactions_(0)
        , rollback_(false)
    {
    }

    connection_impl(string const& dsn, string const& user, string const& pass, long timeout)
        : env_(nullptr)
        , dbc_(nullptr)
        , connected_(false)
        , transactions_(0)
        , rollback_(false)
    {
        allocate();

        try
        {
            connect(dsn, user, pass, timeout);
        }
        catch (...)
        {
            deallocate();
            throw;
        }
    }

    connection_impl(string const& connection_string, long timeout)
        : env_(nullptr)
        , dbc_(nullptr)
        , connected_(false)
        , transactions_(0)
        , rollback_(false)
    {
        allocate();

        try
        {
            connect(connection_string, timeout);
        }
        catch (...)
        {
            deallocate();
            throw;
        }
    }

#ifdef NANODBC_HAS_STD_VARIANT
    connection_impl(
        string const& dsn,
        string const& user,
        string const& pass,
        const std::list<attribute>& attributes)
        : env_(nullptr)
        , dbc_(nullptr)
        , connected_(false)
        , transactions_(0)
        , rollback_(false)
    {
        allocate();

        try
        {
            connect(dsn, user, pass, attributes);
        }
        catch (...)
        {
            deallocate();
            throw;
        }
    }

    connection_impl(string const& connection_string, std::list<attribute> attributes)
        : env_(nullptr)
        , dbc_(nullptr)
        , connected_(false)
        , transactions_(0)
        , rollback_(false)
    {
        allocate();
        try
        {
            connect(connection_string, attributes);
        }
        catch (...)
        {
            deallocate();
            throw;
        }
    }
#endif

    ~connection_impl() noexcept
    {
        try
        {
            disconnect();
        }
        catch (...)
        {
            // ignore exceptions
        }

        try
        {
            deallocate();
        }
        catch (...)
        {
            // ignore exceptions
        }
    }

    void allocate()
    {
        allocate_env_handle(env_);
        allocate_dbc_handle(dbc_, env_);
    }

    void deallocate()
    {
        deallocate_handle(dbc_, SQL_HANDLE_DBC);
        deallocate_handle(env_, SQL_HANDLE_ENV);
    }

    void set_attribute(long const& attr, long const& size, const void* buffer)
    {
        RETCODE rc;

        NANODBC_CALL_RC(SQLSetConnectAttr, rc, dbc_, attr, (SQLPOINTER)(buffer), size);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
    }

#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_EVENT)
    void enable_async(void* event_handle)
    {
        NANODBC_ASSERT(dbc_);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLSetConnectAttr,
            rc,
            dbc_,
            SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
            (SQLPOINTER)SQL_ASYNC_DBC_ENABLE_ON,
            SQL_IS_INTEGER);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);

        NANODBC_CALL_RC(
            SQLSetConnectAttr, rc, dbc_, SQL_ATTR_ASYNC_DBC_EVENT, event_handle, SQL_IS_POINTER);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
    }

    void async_complete()
    {
        NANODBC_ASSERT(dbc_);

        RETCODE rc, arc;
        NANODBC_CALL_RC(SQLCompleteAsync, rc, SQL_HANDLE_DBC, dbc_, &arc);
        if (!success(rc) || !success(arc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);

        connected_ = true;

        NANODBC_CALL_RC(
            SQLSetConnectAttr,
            rc,
            dbc_,
            SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
            (SQLPOINTER)SQL_ASYNC_DBC_ENABLE_OFF,
            SQL_IS_INTEGER);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
    }
#endif // !NANODBC_DISABLE_ASYNC && SQL_ATTR_ASYNC_DBC_EVENT

    RETCODE connect(
        string const& dsn,
        string const& user,
        string const& pass,
        long timeout,
        void* event_handle = nullptr)
    {
        std::list<attribute> attributes;
        // Avoid to set the timeout to 0 (no timeout).
        // This is a workaround for the Oracle ODBC Driver (11.1), as this
        // operation is not supported by the Driver.
        if (timeout != 0)
        {
            attributes.push_back(
                {SQL_ATTR_LOGIN_TIMEOUT, SQL_IS_UINTEGER, (std::uintptr_t)timeout});
        }
#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_EVENT)
        if (event_handle != nullptr)
        {
            attributes.push_back(
                {SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
                 SQL_IS_UINTEGER,
                 (std::uintptr_t)SQL_ASYNC_DBC_ENABLE_ON});
            attributes.push_back(
                {SQL_ATTR_ASYNC_DBC_EVENT, SQL_IS_POINTER, (std::uintptr_t)event_handle});
        }
#endif
        return this->connect(dsn, user, pass, attributes);
    }

    RETCODE
    connect(
        string const& dsn,
        string const& user,
        string const& pass,
        std::list<attribute> const& attributes)
    {
        allocate_env_handle(env_);
        disconnect();

        deallocate_handle(dbc_, SQL_HANDLE_DBC);
        allocate_dbc_handle(dbc_, env_);

        bool is_async = false;
        for (const attribute& attr : attributes)
        {
            if (attr.value_ptr_ == nullptr)
            {
                continue;
            }
#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)
            if (attr.attribute_ == SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE &&
                attr.value_ptr_ == (void*)(std::intptr_t)SQL_ASYNC_DBC_ENABLE_ON)
            {
                is_async = true;
            }
#endif
            this->set_attribute(attr.attribute_, attr.string_length_, attr.value_ptr_);
        }

        RETCODE rc;

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLConnect),
            rc,
            dbc_,
            (NANODBC_SQLCHAR*)dsn.c_str(),
            SQL_NTS,
            !user.empty() ? (NANODBC_SQLCHAR*)user.c_str() : nullptr,
            SQL_NTS,
            !pass.empty() ? (NANODBC_SQLCHAR*)pass.c_str() : nullptr,
            SQL_NTS);
        if (!success(rc) && (!is_async || rc != SQL_STILL_EXECUTING))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);

        connected_ = success(rc);

        return rc;
    }

    RETCODE
    connect(string const& connection_string, long timeout, void* event_handle = nullptr)
    {
        std::list<attribute> attributes;
        // Avoid to set the timeout to 0 (no timeout).
        // This is a workaround for the Oracle ODBC Driver (11.1), as this
        // operation is not supported by the Driver.
        if (timeout != 0)
        {
            attributes.push_back(
                {SQL_ATTR_LOGIN_TIMEOUT, SQL_IS_UINTEGER, (std::uintptr_t)timeout});
        }
#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_EVENT)
        if (event_handle != nullptr)
        {
            attributes.push_back(
                {SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
                 SQL_IS_UINTEGER,
                 (std::uintptr_t)SQL_ASYNC_DBC_ENABLE_ON});
            attributes.push_back(
                {SQL_ATTR_ASYNC_DBC_EVENT, SQL_IS_POINTER, (std::uintptr_t)event_handle});
        }
#endif
        return this->connect(connection_string, attributes);
    }

    RETCODE
    connect(string const& connection_string, std::list<attribute> const& attributes)
    {
        allocate_env_handle(env_);
        disconnect();

        deallocate_handle(dbc_, SQL_HANDLE_DBC);
        allocate_dbc_handle(dbc_, env_);

        bool is_async = false;
        for (const attribute& attr : attributes)
        {
            if (attr.value_ptr_ == nullptr)
            {
                continue;
            }
#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)
            if (attr.attribute_ == SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE &&
                attr.value_ptr_ == (void*)(std::intptr_t)SQL_ASYNC_DBC_ENABLE_ON)
            {
                is_async = true;
            }
#endif
            this->set_attribute(attr.attribute_, attr.string_length_, attr.value_ptr_);
        }

        RETCODE rc;

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLDriverConnect),
            rc,
            dbc_,
            nullptr,
            (NANODBC_SQLCHAR*)connection_string.c_str(),
            SQL_NTS,
            nullptr,
            0,
            nullptr,
            SQL_DRIVER_NOPROMPT);
        if (!success(rc) && (!is_async || rc != SQL_STILL_EXECUTING))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);

        connected_ = success(rc);

        return rc;
    }

    bool connected() const { return connected_; }

    void disconnect()
    {
        if (connected())
        {
            RETCODE rc;
            NANODBC_CALL_RC(SQLDisconnect, rc, dbc_);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
        }
        connected_ = false;
    }

    std::size_t transactions() const { return transactions_; }

    void* native_dbc_handle() const { return dbc_; }

    void* native_env_handle() const { return env_; }

    template <class T>
    T get_info(short info_type) const
    {
        return get_info_impl<T>(info_type);
    }
    string dbms_name() const;

    string dbms_version() const;

    string driver_name() const;

    string driver_version() const;

    string database_name() const;

    string catalog_name() const
    {
        NANODBC_SQLCHAR name[SQL_MAX_OPTION_STRING_LENGTH] = {0};
        SQLINTEGER length(0);
        RETCODE rc;
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetConnectAttr),
            rc,
            dbc_,
            SQL_ATTR_CURRENT_CATALOG,
            name,
            sizeof(name) / sizeof(NANODBC_SQLCHAR),
            &length);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
        return {&name[0], &name[size(name)]};
    }

    std::size_t ref_transaction() { return ++transactions_; }

    std::size_t unref_transaction()
    {
        if (transactions_ > 0)
            --transactions_;
        return transactions_;
    }

    bool rollback() const { return rollback_; }

    void rollback(bool onoff) { rollback_ = onoff; }

private:
    template <class T, typename std::enable_if<!is_string<T>::value, int>::type = 0>
    T get_info_impl(short info_type) const;

    template <class T, typename std::enable_if<is_string<T>::value, int>::type = 0>
    T get_info_impl(short info_type) const;

    HENV env_;
    HDBC dbc_;
    bool connected_;
    std::size_t transactions_;
    bool rollback_; // if true, this connection is marked for eventual transaction rollback
};

template <class T, typename std::enable_if<!is_string<T>::value, int>::type>
T connection::connection_impl::get_info_impl(short info_type) const
{
    T value = 0;
    RETCODE rc;
    NANODBC_CALL_RC(NANODBC_FUNC(SQLGetInfo), rc, dbc_, info_type, &value, 0, nullptr);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
    return value;
}

template <class T, typename std::enable_if<is_string<T>::value, int>::type>
T connection::connection_impl::get_info_impl(short info_type) const
{
    NANODBC_SQLCHAR value[1024] = {0};
    SQLSMALLINT length(0);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetInfo),
        rc,
        dbc_,
        info_type,
        value,
        sizeof(value) / sizeof(NANODBC_SQLCHAR),
        &length);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(dbc_, SQL_HANDLE_DBC);
    return {&value[0], &value[size(value)]};
}

string connection::connection_impl::dbms_name() const
{
    return get_info<string>(SQL_DBMS_NAME);
}

string connection::connection_impl::dbms_version() const
{
    return get_info<string>(SQL_DBMS_VER);
}

string connection::connection_impl::driver_name() const
{
    return get_info<string>(SQL_DRIVER_NAME);
}

string connection::connection_impl::driver_version() const
{
    return get_info<string>(SQL_DRIVER_VER);
}

string connection::connection_impl::database_name() const
{
    return get_info<string>(SQL_DATABASE_NAME);
}

template string connection::get_info(short info_type) const;
template unsigned short connection::get_info(short info_type) const;
template uint32_t connection::get_info(short info_type) const;
template uint64_t connection::get_info(short info_type) const;

} // namespace nanodbc

// clang-format off
// 88888888888                                                  888    d8b                             8888888                        888
//     888                                                      888    Y8P                               888                          888
//     888                                                      888                                      888                          888
//     888  888d888 8888b.  88888b.  .d8888b   8888b.   .d8888b 888888 888  .d88b.  88888b.              888   88888b.d88b.  88888b.  888
//     888  888P"      "88b 888 "88b 88K          "88b d88P"    888    888 d88""88b 888 "88b             888   888 "888 "88b 888 "88b 888
//     888  888    .d888888 888  888 "Y8888b. .d888888 888      888    888 888  888 888  888             888   888  888  888 888  888 888
//     888  888    888  888 888  888      X88 888  888 Y88b.    Y88b.  888 Y88..88P 888  888             888   888  888  888 888 d88P 888
//     888  888    "Y888888 888  888  88888P' "Y888888  "Y8888P  "Y888 888  "Y88P"  888  888           8888888 888  888  888 88888P"  888
//                                                                                                                           888
//                                                                                                                           888
//                                                                                                                           888
// MARK: Transaction Impl -
// clang-format on

namespace nanodbc
{

class transaction::transaction_impl
{
public:
    transaction_impl(transaction_impl const&) = delete;
    transaction_impl& operator=(transaction_impl const&) = delete;

    explicit transaction_impl(class connection conn)
        : conn_(std::move(conn))
        , committed_(false)
    {
        if (conn_.transactions() == 0 && conn_.connected())
        {
            RETCODE rc;
            NANODBC_CALL_RC(
                SQLSetConnectAttr,
                rc,
                conn_.native_dbc_handle(),
                SQL_ATTR_AUTOCOMMIT,
                (SQLPOINTER)SQL_AUTOCOMMIT_OFF,
                SQL_IS_UINTEGER);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(conn_.native_dbc_handle(), SQL_HANDLE_DBC);
        }
        conn_.ref_transaction();
    }

    ~transaction_impl() noexcept
    {
        try
        {
            if (!committed_)
            {
                conn_.rollback(true);
                conn_.unref_transaction();
            }

            if (conn_.transactions() == 0 && conn_.connected())
            {
                if (conn_.rollback())
                {
                    NANODBC_CALL(
                        SQLEndTran, SQL_HANDLE_DBC, conn_.native_dbc_handle(), SQL_ROLLBACK);
                    conn_.rollback(false);
                }

                NANODBC_CALL(
                    SQLSetConnectAttr,
                    conn_.native_dbc_handle(),
                    SQL_ATTR_AUTOCOMMIT,
                    (SQLPOINTER)SQL_AUTOCOMMIT_ON,
                    SQL_IS_UINTEGER);
            }
        }
        catch (...)
        {
            // ignore exceptions
        }
    }

    void commit()
    {
        if (committed_)
            return;
        committed_ = true;
        if (conn_.unref_transaction() == 0 && conn_.connected())
        {
            RETCODE rc;
            NANODBC_CALL_RC(SQLEndTran, rc, SQL_HANDLE_DBC, conn_.native_dbc_handle(), SQL_COMMIT);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(conn_.native_dbc_handle(), SQL_HANDLE_DBC);
        }
    }

    void rollback() noexcept
    {
        if (committed_)
            return;
        conn_.rollback(true);
    }

    class connection& connection() { return conn_; }

    const class connection& connection() const { return conn_; }

private:
    class connection conn_;
    bool committed_;
};

} // namespace nanodbc

// clang-format off
//  .d8888b.  888             888                                            888              8888888                        888
// d88P  Y88b 888             888                                            888                888                          888
// Y88b.      888             888                                            888                888                          888
//  "Y888b.   888888  8888b.  888888 .d88b.  88888b.d88b.   .d88b.  88888b.  888888             888   88888b.d88b.  88888b.  888
//     "Y88b. 888        "88b 888   d8P  Y8b 888 "888 "88b d8P  Y8b 888 "88b 888                888   888 "888 "88b 888 "88b 888
//       "888 888    .d888888 888   88888888 888  888  888 88888888 888  888 888                888   888  888  888 888  888 888
// Y88b  d88P Y88b.  888  888 Y88b. Y8b.     888  888  888 Y8b.     888  888 Y88b.              888   888  888  888 888 d88P 888
//  "Y8888P"   "Y888 "Y888888  "Y888 "Y8888  888  888  888  "Y8888  888  888  "Y888           8888888 888  888  888 88888P"  888
//                                                                                                                  888
//                                                                                                                  888
//                                                                                                                  888
// MARK: Statement Impl -
// clang-format on

namespace nanodbc
{

class statement::statement_impl
{
public:
    statement_impl(statement_impl const&) = delete;
    statement_impl& operator=(statement_impl const&) = delete;

    statement_impl()
        : stmt_(nullptr)
        , open_(false)
        , conn_()
        , bind_len_or_null_()
#if defined(NANODBC_DO_ASYNC_IMPL)
        , async_(false)
        , async_enabled_(false)
        , async_event_(nullptr)
#endif
#ifndef NANODBC_DISABLE_MSSQL_TVP
        , tvp_data_()
        , open_tvp_(false)
#endif
    {
    }

    explicit statement_impl(class connection& conn)
        : stmt_(nullptr)
        , open_(false)
        , conn_()
        , bind_len_or_null_()
        , wide_string_data_()
        , string_data_()
        , binary_data_()
#if defined(NANODBC_DO_ASYNC_IMPL)
        , async_(false)
        , async_enabled_(false)
        , async_event_(nullptr)
#endif
#ifndef NANODBC_DISABLE_MSSQL_TVP
        , tvp_data_()
        , open_tvp_(false)
#endif
    {
        open(conn);
    }

    explicit statement_impl(class connection& conn, const std::list<attribute>& attributes)
        : stmt_(nullptr)
        , open_(false)
        , conn_()
        , bind_len_or_null_()
        , wide_string_data_()
        , string_data_()
        , binary_data_()
#if defined(NANODBC_DO_ASYNC_IMPL)
        , async_(false)
        , async_enabled_(false)
        , async_event_(nullptr)
#endif
#ifndef NANODBC_DISABLE_MSSQL_TVP
        , tvp_data_()
        , open_tvp_(false)
#endif
    {
        open(conn, attributes);
    }

    statement_impl(class connection& conn, string const& query, long timeout)
        : stmt_(nullptr)
        , open_(false)
        , conn_()
        , bind_len_or_null_()
        , wide_string_data_()
        , string_data_()
        , binary_data_()
#if defined(NANODBC_DO_ASYNC_IMPL)
        , async_(false)
        , async_enabled_(false)
        , async_event_(nullptr)
#endif
#ifndef NANODBC_DISABLE_MSSQL_TVP
        , tvp_data_()
        , open_tvp_(false)
#endif
    {
        prepare(conn, query, timeout);
    }

    ~statement_impl() noexcept
    {
        try
        {
            if (open() && connected())
            {
                NANODBC_CALL(SQLCancel, stmt_);
                reset_parameters();
                deallocate_handle(stmt_, SQL_HANDLE_STMT);
            }
        }
        catch (...)
        {
            // ignore exceptions
        }
    }

    void open(class connection& conn)
    {
        if (open() && connected() && conn_.native_dbc_handle() == conn.native_dbc_handle())
        {
            return;
        }
        close();
        RETCODE rc;
        NANODBC_CALL_RC(SQLAllocHandle, rc, SQL_HANDLE_STMT, conn.native_dbc_handle(), &stmt_);
        open_ = success(rc);
        if (!open_)
        {
            if (!stmt_)
                NANODBC_THROW_DATABASE_ERROR(conn.native_dbc_handle(), SQL_HANDLE_DBC);
            else
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }
        conn_ = conn;
    }

    void open(class connection& conn, std::list<attribute> const& attributes)
    {
        open(conn);
        for (const attribute& attr : attributes)
        {
            if (attr.value_ptr_ == nullptr)
            {
                continue;
            }
            this->set_attribute(attr.attribute_, attr.string_length_, attr.value_ptr_);
        }
    }

    bool open() const { return open_; }

    bool connected() const { return conn_.connected(); }

    const class connection& connection() const { return conn_; }

    class connection& connection() { return conn_; }

    void* native_statement_handle() const { return stmt_; }

    void close()
    {
#ifndef NANODBC_DISABLE_MSSQL_TVP
        tvp_data_.clear();
#endif

        if (open() && connected())
        {
            RETCODE rc;
            NANODBC_CALL_RC(SQLCancel, rc, stmt_);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

            reset_parameters();
            deallocate_handle(stmt_, SQL_HANDLE_STMT);
        }

        open_ = false;
        stmt_ = nullptr;
    }

#ifndef NANODBC_DISABLE_MSSQL_TVP
    void open_tvp(
        table_valued_parameter& tvp,
        short param_index,
        bound_parameter& param,
        std::vector<null_type>** bind_len_or_null)
    {
        if (open_tvp_)
            throw programming_error("tvp already opened");

        prepare_bind(param_index, 1, PARAM_IN, param);
        if (param.type_ != SQL_SS_TABLE)
            throw programming_error("invalid tvp param type");

        tvp_data_.emplace(std::make_pair(param_index, tvp));
        *bind_len_or_null = &bind_len_or_null_[param_index];
        open_tvp_ = true;
    }

    void close_tvp()
    {
        if (!open_tvp_)
            throw programming_error("tvp already closed");
        open_tvp_ = false;
    }
#endif

    void cancel()
    {
        RETCODE rc;
        NANODBC_CALL_RC(SQLCancel, rc, stmt_);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
    }

    void prepare(class connection& conn, string const& query, long timeout)
    {
        open(conn);
        prepare(query, timeout);
    }

    RETCODE prepare(string const& query, long timeout, void* event_handle = nullptr)
    {
        if (!open())
            throw programming_error("statement has no associated open connection");

#if defined(NANODBC_DO_ASYNC_IMPL)
        if (event_handle == nullptr)
            disable_async();
        else
            enable_async(event_handle);
#endif

        RETCODE rc;
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLPrepare),
            rc,
            stmt_,
            (NANODBC_SQLCHAR*)query.c_str(),
            (SQLINTEGER)query.size());
        if (!success(rc) && rc != SQL_STILL_EXECUTING)
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

        this->timeout(timeout);

        return rc;
    }

    void timeout(long timeout)
    {
        // some drivers don't support timeout for statements,
        // so only raise the error if a non-default timeout was requested.
        try
        {
            this->set_attribute(SQL_ATTR_QUERY_TIMEOUT, 0, &timeout);
        }
        catch (...)
        {
            if (timeout != 0)
            {
                throw;
            }
        }
        return;
    }

    void set_attribute(long const& attr, long const& size, const void* buffer)
    {
        RETCODE rc;

        NANODBC_CALL_RC(SQLSetStmtAttr, rc, stmt_, attr, (SQLPOINTER)(buffer), size);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
    }

#if defined(NANODBC_DO_ASYNC_IMPL)
    void enable_async(void* event_handle)
    {
        RETCODE rc;
        if (!async_enabled_)
        {
            NANODBC_CALL_RC(
                SQLSetStmtAttr,
                rc,
                stmt_,
                SQL_ATTR_ASYNC_ENABLE,
                (SQLPOINTER)SQL_ASYNC_ENABLE_ON,
                SQL_IS_INTEGER);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
            async_enabled_ = true;
        }

        if (async_event_ != event_handle)
        {
            NANODBC_CALL_RC(
                SQLSetStmtAttr, rc, stmt_, SQL_ATTR_ASYNC_STMT_EVENT, event_handle, SQL_IS_POINTER);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
            async_event_ = event_handle;
        }
    }

    void disable_async() const
    {
        if (async_enabled_)
        {
            RETCODE rc;
            NANODBC_CALL_RC(
                SQLSetStmtAttr,
                rc,
                stmt_,
                SQL_ATTR_ASYNC_ENABLE,
                (SQLPOINTER)SQL_ASYNC_ENABLE_OFF,
                SQL_IS_INTEGER);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
            async_enabled_ = false;
        }
    }

    bool async_helper(RETCODE rc)
    {
        if (rc == SQL_STILL_EXECUTING)
        {
            async_ = true;
            return true;
        }
        else if (success(rc))
        {
            async_ = false;
            return false;
        }
        else
        {
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }
    }

    bool async_prepare(string const& query, void* event_handle, long timeout)
    {
        return async_helper(prepare(query, timeout, event_handle));
    }

    bool async_execute_direct(
        class connection& conn,
        void* event_handle,
        string const& query,
        long batch_operations,
        long timeout,
        statement& statement)
    {
        return async_helper(
            just_execute_direct(conn, query, batch_operations, timeout, statement, event_handle));
    }

    bool
    async_execute(void* event_handle, long batch_operations, long timeout, statement& statement)
    {
        return async_helper(just_execute(batch_operations, timeout, statement, event_handle));
    }

    void call_complete_async()
    {
        if (async_)
        {
            RETCODE rc, arc;
            NANODBC_CALL_RC(SQLCompleteAsync, rc, SQL_HANDLE_STMT, stmt_, &arc);
            if (!success(rc) || !success(arc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }
    }

    result complete_execute(long batch_operations, statement& statement)
    {
        call_complete_async();

        return result(statement, batch_operations);
    }

    void complete_prepare() { call_complete_async(); }

#endif
    result execute_direct(
        class connection& conn,
        string const& query,
        long batch_operations,
        long timeout,
        statement& statement)
    {
        const batch_ops array_ops(batch_operations);
        return execute_direct(conn, query, array_ops, timeout, statement);
    }

    result execute_direct(
        class connection& conn,
        string const& query,
        batch_ops const& array_sizes,
        long timeout,
        statement& statement)
    {
#ifdef NANODBC_ENABLE_WORKAROUND_NODATA
        const RETCODE rc = just_execute_direct(conn, query, array_sizes, timeout, statement);
        if (rc == SQL_NO_DATA)
            return result();
#else
        just_execute_direct(conn, query, array_sizes, timeout, statement);
#endif
        return {statement, array_sizes.rowset_size};
    }

    RETCODE just_execute_direct(
        class connection& conn,
        string const& query,
        batch_ops const& array_sizes,
        long timeout,
        statement&, // statement
        void* event_handle = nullptr)
    {
        open(conn);

#if defined(NANODBC_DO_ASYNC_IMPL)
        if (event_handle == nullptr)
            disable_async();
        else
            enable_async(event_handle);
#endif

        RETCODE rc;
        if (array_sizes.parameter_array_length > 0)
        {
            NANODBC_CALL_RC(
                SQLSetStmtAttr,
                rc,
                stmt_,
                SQL_ATTR_PARAMSET_SIZE,
                (SQLPOINTER)(std::intptr_t)array_sizes.parameter_array_length,
                0);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }

        this->timeout(timeout);

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLExecDirect), rc, stmt_, (NANODBC_SQLCHAR*)query.c_str(), SQL_NTS);
        if (!success(rc) && rc != SQL_NO_DATA && rc != SQL_STILL_EXECUTING)
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

        return rc;
    }

    result execute(long batch_operations, long timeout, statement& statement)
    {
#ifdef NANODBC_ENABLE_WORKAROUND_NODATA
        const RETCODE rc = just_execute(batch_operations, timeout, statement);
        if (rc == SQL_NO_DATA)
            return result();
#else
        just_execute(batch_operations, timeout, statement);
#endif
        return {statement, batch_operations};
    }

    RETCODE just_execute(
        long batch_operations,
        long timeout,
        statement& /*statement*/,
        void* event_handle = nullptr)
    {
#ifndef NANODBC_DISABLE_MSSQL_TVP
        if (!tvp_data_.empty() && 1 != batch_operations)
            throw programming_error("cannot use batch operation when using tvp");
#endif

        RETCODE rc;

        if (open())
        {
            // The ODBC cursor must be closed before subsequent executions, as described
            // here
            // http://msdn.microsoft.com/en-us/library/windows/desktop/ms713584%28v=vs.85%29.aspx
            //
            // However, we don't necessarily want to call SQLCloseCursor() because that
            // will cause an invalid cursor state in the case that no cursor is currently open.
            // A better solution is to use SQLFreeStmt() with the SQL_CLOSE option, which has
            // the same effect without the undesired limitations.
            NANODBC_CALL_RC(SQLFreeStmt, rc, stmt_, SQL_CLOSE);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }

#if defined(NANODBC_DO_ASYNC_IMPL)
        if (event_handle == nullptr)
            disable_async();
        else
            enable_async(event_handle);
#endif

        NANODBC_CALL_RC(
            SQLSetStmtAttr,
            rc,
            stmt_,
            SQL_ATTR_PARAMSET_SIZE,
            (SQLPOINTER)(std::intptr_t)batch_operations,
            0);
        if (!success(rc) && rc != SQL_NO_DATA)
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

        this->timeout(timeout);

        NANODBC_CALL_RC(SQLExecute, rc, stmt_);
        if (!success(rc) && rc != SQL_NO_DATA && rc != SQL_STILL_EXECUTING)
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

        return rc;
    }

    result procedure_columns(
        string const& catalog,
        string const& schema,
        string const& procedure,
        string const& column,
        statement& statement)
    {
        if (!open())
            throw programming_error("statement has no associated open connection");

#if defined(NANODBC_DO_ASYNC_IMPL)
        disable_async();
#endif

        RETCODE rc;
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLProcedureColumns),
            rc,
            stmt_,
            (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
            (catalog.empty() ? 0 : SQL_NTS),
            (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
            (schema.empty() ? 0 : SQL_NTS),
            (NANODBC_SQLCHAR*)procedure.c_str(),
            SQL_NTS,
            (NANODBC_SQLCHAR*)(column.empty() ? nullptr : column.c_str()),
            (column.empty() ? 0 : SQL_NTS));

        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);

        return {statement, 1};
    }

    long affected_rows() const
    {
        SQLLEN rows;
        RETCODE rc;
        NANODBC_CALL_RC(SQLRowCount, rc, stmt_, &rows);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        NANODBC_ASSERT(rows <= static_cast<SQLLEN>(std::numeric_limits<long>::max()));
        return static_cast<long>(rows);
    }

    short columns() const
    {
        SQLSMALLINT cols;
        RETCODE rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
        disable_async();
#endif

        NANODBC_CALL_RC(SQLNumResultCols, rc, stmt_, &cols);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        return cols;
    }

    void reset_parameters() noexcept
    {
        param_descr_data_.clear();
        NANODBC_CALL(SQLFreeStmt, stmt_, SQL_RESET_PARAMS);
    }

    short parameters() const
    {
        SQLSMALLINT params;
        RETCODE rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
        disable_async();
#endif

        NANODBC_CALL_RC(SQLNumParams, rc, stmt_, &params);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        return params;
    }

    unsigned long parameter_size(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<unsigned long>(param_descr_data_.at(param_index).size_);
        }

        describe_parameters(param_index);
        const SQLULEN& param_size = param_descr_data_.at(param_index).size_;
        NANODBC_ASSERT(
            param_size < static_cast<SQLULEN>(std::numeric_limits<unsigned long>::max()));
        return static_cast<unsigned long>(param_size);
    }

    short parameter_scale(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<short>(param_descr_data_.at(param_index).scale_);
        }

        describe_parameters(param_index);
        const SQLSMALLINT& param_scale = param_descr_data_.at(param_index).scale_;
        NANODBC_ASSERT(param_scale < static_cast<SQLULEN>(std::numeric_limits<short>::max()));
        return static_cast<short>(param_scale);
    }

    short parameter_type(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<short>(param_descr_data_.at(param_index).type_);
        }

        describe_parameters(param_index);
        const SQLSMALLINT& param_type = param_descr_data_.at(param_index).type_;
        NANODBC_ASSERT(param_type < static_cast<SQLULEN>(std::numeric_limits<short>::max()));
        return static_cast<short>(param_type);
    }

    static SQLSMALLINT param_type_from_direction(param_direction direction)
    {
        switch (direction)
        {
        case PARAM_IN:
            return SQL_PARAM_INPUT;
            break;
        case PARAM_OUT:
            return SQL_PARAM_OUTPUT;
            break;
        case PARAM_INOUT:
            return SQL_PARAM_INPUT_OUTPUT;
            break;
        case PARAM_RETURN:
            return SQL_PARAM_OUTPUT;
            break;
        default:
            NANODBC_ASSERT(false);
            throw programming_error("unrecognized param_direction value");
        }
    }

    // initializes bind_len_or_null_ and gets information for bind
    void prepare_bind(
        short param_index,
        std::size_t batch_size,
        param_direction direction,
        bound_parameter& param)
    {
        NANODBC_ASSERT(param_index >= 0);

#if defined(NANODBC_DO_ASYNC_IMPL)
        disable_async();
#endif

        if (!param_descr_data_.count(param_index))
        {
            describe_parameters(param_index);
        }
        param.index_ = param_index;
        param.type_ = param_descr_data_[param_index].type_;
        param.size_ = param_descr_data_[param_index].size_;
        param.scale_ = param_descr_data_[param_index].scale_;
        param.iotype_ = param_type_from_direction(direction);

        if (!bind_len_or_null_.count(param_index))
            bind_len_or_null_[param_index] = std::vector<null_type>();
        std::vector<null_type>().swap(bind_len_or_null_[param_index]);

        // ODBC weirdness: this must be at least 8 elements in size
        const std::size_t indicator_size = batch_size > 8 ? batch_size : 8;
        bind_len_or_null_[param_index].reserve(indicator_size);
        bind_len_or_null_[param_index].assign(indicator_size, SQL_NULL_DATA);

        NANODBC_ASSERT(param.index_ == param_index);
        NANODBC_ASSERT(param.iotype_ > 0);
    }

    // calls actual ODBC bind parameter function
    template <class T, typename std::enable_if<!is_character<T>::value, int>::type = 0>
    void bind_parameter(bound_parameter const& param, bound_buffer<T>& buffer)
    {
        NANODBC_ASSERT(buffer.value_size_ > 0 || param.size_ > 0);

#ifndef NANODBC_DISABLE_MSSQL_TVP
        if (open_tvp_)
            throw programming_error("cannot bind parameter, close tvp first");
#endif

        auto value_size{buffer.value_size_};
        if (value_size == 0)
            value_size = param.size_;

        auto param_size{param.size_};
        if (value_size > param_size)
        {
            // Parameter size reported by SQLDescribeParam for Large Objects:
            // - For SQL VARBINARY(MAX), it is Zero which actually means SQL_SS_LENGTH_UNLIMITED.
            // - For SQL UDT (eg. GEOMETRY), it may be driver-specific max limit (eg. SQL Server is
            // DBMAXCHAR=8000 bytes).
            // See MSDN for details
            // https://docs.microsoft.com/en-us/sql/relational-databases/native-client/odbc/large-clr-user-defined-types-odbc
            //
            // If bound value is larger than parameter size, we force SQL_SS_LENGTH_UNLIMITED.
            param_size = SQL_SS_LENGTH_UNLIMITED;
        }

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_,               // handle
            param.index_ + 1,    // parameter number
            param.iotype_,       // input or output type
            sql_ctype<T>::value, // value type
            param.type_,         // parameter type
            param_size,          // column size ignored for many types, but needed for strings
            param.scale_,        // decimal digits
            (SQLPOINTER)buffer.values_, // parameter value
            value_size,                 // buffer length
            bind_len_or_null_[param.index_].data());

        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
    }

    // Supports code like: query.bind(0, std_string.c_str())
    // In this case, we need to pass nullptr to the final parameter of SQLBindParameter().
    template <class T, typename std::enable_if<is_character<T>::value, int>::type = 0>
    void bind_parameter(bound_parameter const& param, bound_buffer<T>& buffer)
    {
#ifndef NANODBC_DISABLE_MSSQL_TVP
        if (open_tvp_)
            throw programming_error("cannot bind parameter, close tvp first");
#endif

        auto const buffer_size = buffer.value_size_ > 0 ? buffer.value_size_ : param.size_;

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_,               // handle
            param.index_ + 1,    // parameter number
            param.iotype_,       // input or output type
            sql_ctype<T>::value, // value type
            param.type_,         // parameter type
            param.size_,         // column size ignored for many types, but needed for strings
            param.scale_,        // decimal digits
            (SQLPOINTER)buffer.values_, // parameter value
            buffer_size,                // buffer length
            (buffer.size_ <= 1 ? nullptr : bind_len_or_null_[param.index_].data()));

        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
    }

    template <class T>
    void bind(
        param_direction direction,
        short param_index,
        T const* values,
        std::size_t batch_size,
        bool const* nulls = nullptr,
        T const* null_sentry = nullptr);

    // handles multiple binary values
    void bind(
        param_direction direction,
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        bool const* nulls = nullptr,
        uint8_t const* null_sentry = nullptr)
    {
        std::size_t batch_size = values.size();
        bound_parameter param;
        prepare_bind(param_index, batch_size, direction, param);

        size_t max_length = 0;
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            max_length = std::max(values[i].size(), max_length);
        }
        binary_data_[param_index] = std::vector<uint8_t>(batch_size * max_length, 0);
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            std::copy(
                values[i].begin(),
                values[i].end(),
                binary_data_[param_index].data() + (i * max_length));
        }

        if (null_sentry)
        {
            for (std::size_t i = 0; i < batch_size; ++i)
                if (!std::equal(values[i].begin(), values[i].end(), null_sentry))
                {
                    bind_len_or_null_[param_index][i] = values[i].size();
                }
        }
        else if (nulls)
        {
            for (std::size_t i = 0; i < batch_size; ++i)
            {
                if (!nulls[i])
                    bind_len_or_null_[param_index][i] = values[i].size(); // null terminated
            }
        }
        else
        {
            for (std::size_t i = 0; i < batch_size; ++i)
            {
                bind_len_or_null_[param_index][i] = values[i].size();
            }
        }
        bound_buffer<uint8_t> buffer(binary_data_[param_index].data(), batch_size, max_length);
        bind_parameter(param, buffer);
    }

    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        param_direction direction,
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        bool const* nulls = nullptr,
        T const* null_sentry = nullptr);

    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        param_direction direction,
        short param_index,
        std::vector<T> const& values,
        bool const* nulls = nullptr,
        typename T::value_type const* null_sentry = nullptr);

    // handles multiple null values
    void bind_null(short param_index, std::size_t batch_size)
    {
        bound_parameter param;
        prepare_bind(param_index, batch_size, PARAM_IN, param);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_,
            param.index_ + 1, // parameter number
            param.iotype_,    // input or output typ,
            SQL_C_CHAR,
            param.type_, // parameter type
            param.size_, // column size ignored for many types, but needed for string,
            0,           // decimal digits
            nullptr,     // null value
            0,           // buffe length
            bind_len_or_null_[param.index_].data());
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
    }

    void describe_parameters(const short param_index)
    {
        RETCODE rc;
        SQLSMALLINT nullable; // unused
#if defined(NANODBC_DO_ASYNC_IMPL)
        disable_async();
#endif
        NANODBC_CALL_RC(
            SQLDescribeParam,
            rc,
            stmt_,
            static_cast<SQLUSMALLINT>(param_index + 1),
            &param_descr_data_[param_index].type_,
            &param_descr_data_[param_index].size_,
            &param_descr_data_[param_index].scale_,
            &nullable);
        if (!success(rc))
        {
            param_descr_data_.erase(param_index);
            NANODBC_THROW_DATABASE_ERROR(stmt_, SQL_HANDLE_STMT);
        }
    }

    void describe_parameters(
        const std::vector<short>& idx,
        const std::vector<short>& type,
        const std::vector<unsigned long>& size,
        const std::vector<short>& scale)
    {

        if (idx.size() != type.size() || idx.size() != size.size() || idx.size() != scale.size())
            throw programming_error("parameter description arrays are of different size");

        for (std::size_t i = 0; i < idx.size(); ++i)
        {
            param_descr_data_[idx[i]].type_ = static_cast<SQLSMALLINT>(type[i]);
            param_descr_data_[idx[i]].size_ = static_cast<SQLULEN>(size[i]);
            param_descr_data_[idx[i]].scale_ = static_cast<SQLSMALLINT>(scale[i]);
            param_descr_data_[idx[i]].index_ = static_cast<SQLUSMALLINT>(i);
            param_descr_data_[idx[i]].iotype_ = PARAM_IN; // not used
        }
    }

    // comparator for null sentry values
    template <class T>
    bool equals(T const& lhs, T const& rhs)
    {
        return lhs == rhs;
    }

    template <class T>
    std::vector<T>& get_bound_string_data(short param_index);

private:
    HSTMT stmt_;
    bool open_;
    class connection conn_;
    std::map<short, std::vector<null_type>> bind_len_or_null_;
    std::map<short, std::vector<wide_string::value_type>> wide_string_data_;
    std::map<short, std::vector<std::string::value_type>> string_data_;
    std::map<short, std::vector<uint8_t>> binary_data_;
    std::map<short, bound_parameter> param_descr_data_;

#if defined(NANODBC_DO_ASYNC_IMPL)
    bool async_;                 // true if statement is currently in SQL_STILL_EXECUTING mode
    mutable bool async_enabled_; // true if statement currently has SQL_ATTR_ASYNC_ENABLE =
                                 // SQL_ASYNC_ENABLE_ON
    void* async_event_;          // currently active event handle for async notifications
#endif

#ifndef NANODBC_DISABLE_MSSQL_TVP
    std::map<short, table_valued_parameter> tvp_data_;
    bool open_tvp_;
#endif
};

template <class T>
void statement::statement_impl::bind(
    param_direction direction,
    short param_index,
    T const* values,
    std::size_t batch_size,
    bool const* nulls /*= nullptr*/,
    T const* null_sentry /*= nullptr*/)
{
    bound_parameter param;
    prepare_bind(param_index, batch_size, direction, param);

    if (nulls || null_sentry)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
            if ((null_sentry && !equals(values[i], *null_sentry)) || (nulls && !nulls[i]) || !nulls)
                bind_len_or_null_[param_index][i] = param.size_;
    }
    else
    {
        for (std::size_t i = 0; i < batch_size; ++i)
            bind_len_or_null_[param_index][i] = param.size_;
    }

    bound_buffer<T> buffer(values, batch_size);
    bind_parameter(param, buffer);
}

template <class T, typename>
void statement::statement_impl::bind_strings(
    param_direction direction,
    short param_index,
    std::vector<T> const& values,
    bool const* nulls /*= nullptr*/,
    typename T::value_type const* null_sentry /*= nullptr*/)
{
    using string_vector = std::vector<typename T::value_type>;
    string_vector& string_data = get_bound_string_data<typename T::value_type>(param_index);

    size_t const batch_size = values.size();
    bound_parameter param;
    prepare_bind(param_index, batch_size, direction, param);

    size_t max_length = 0;
    for (std::size_t i = 0; i < batch_size; ++i)
    {
        max_length = std::max(values[i].length(), max_length);
    }
    // add space for null terminator
    ++max_length;

    string_data = string_vector(batch_size * max_length, 0);
    for (std::size_t i = 0; i < batch_size; ++i)
    {
        std::copy(values[i].begin(), values[i].end(), string_data.data() + (i * max_length));
    }
    bind_strings(
        direction, param_index, string_data.data(), max_length, batch_size, nulls, null_sentry);
}

template <class T, typename>
void statement::statement_impl::bind_strings(
    param_direction direction,
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    bool const* nulls /*= nullptr*/,
    T const* null_sentry /*= nullptr*/)
{
    bound_parameter param;
    prepare_bind(param_index, batch_size, direction, param);

    if (null_sentry)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            const std::basic_string<T> s_lhs(
                values + i * value_size, values + (i + 1) * value_size);
            const std::basic_string<T> s_rhs(null_sentry);
            if (!equals(s_lhs, s_rhs))
                bind_len_or_null_[param_index][i] = SQL_NTS;
        }
    }
    else if (nulls)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            if (!nulls[i])
                bind_len_or_null_[param_index][i] = SQL_NTS; // null terminated
        }
    }
    else
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            bind_len_or_null_[param_index][i] = SQL_NTS;
        }
    }

    auto const buffer_length = value_size * sizeof(T);
    bound_buffer<T> buffer(values, batch_size, buffer_length);
    bind_parameter(param, buffer);
}

template <>
bool statement::statement_impl::equals(std::string const& lhs, std::string const& rhs)
{
    return std::strncmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
}

template <>
bool statement::statement_impl::equals(const wide_string& lhs, const wide_string& rhs)
{
    // e6059ff3a79062f83256b9d1d3c9c8368798781e
    // Functions like `swprintf()`, `wcsftime()`, `wcsncmp()` can not be used
    // with `u16string` types. Instead, prefers to narrow unicode string to
    // work with them, and then widen them after work has been completed.
    std::string narrow_lhs;
    narrow_lhs.reserve(lhs.size());
    convert(lhs, narrow_lhs);
    std::string narrow_rhs;
    narrow_rhs.reserve(rhs.size());
    convert(rhs, narrow_rhs);
    return equals(narrow_lhs, narrow_rhs);
}

template <>
bool statement::statement_impl::equals(const date& lhs, const date& rhs)
{
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day;
}

template <>
bool statement::statement_impl::equals(const time& lhs, const time& rhs)
{
    return lhs.hour == rhs.hour && lhs.min == rhs.min && lhs.sec == rhs.sec;
}

template <>
bool statement::statement_impl::equals(const timestamp& lhs, const timestamp& rhs)
{
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day &&
           lhs.hour == rhs.hour && lhs.min == rhs.min && lhs.sec == rhs.sec &&
           lhs.fract == rhs.fract;
}

template <>
std::vector<wide_string::value_type>&
statement::statement_impl::get_bound_string_data(short param_index)
{
    return wide_string_data_[param_index];
}

template <>
std::vector<std::string::value_type>&
statement::statement_impl::get_bound_string_data(short param_index)
{
    return string_data_[param_index];
}

} // namespace nanodbc

#ifndef NANODBC_DISABLE_MSSQL_TVP
namespace nanodbc
{

class table_valued_parameter::table_valued_parameter_impl
{
public:
    table_valued_parameter_impl()
        : row_count_(0)
        , param_index_(0)
        , open_(false)
    {
    }

    ~table_valued_parameter_impl() noexcept
    {
        try
        {
            close();
        }
        catch (...)
        {
            // ignore exceptions
        }
    }

    void
    open(table_valued_parameter& tvp, statement& stmt, short param_index, std::size_t row_count)
    {
        bound_parameter param;
        std::vector<null_type>* bind_len_or_null = nullptr;

        close();
        stmt.impl_->open_tvp(tvp, param_index, param, &bind_len_or_null);

        // initialize variables
        open_ = true;
        stmt_ = stmt.impl_;
        row_count_ = row_count;
        param_index_ = param_index;

        prepare_tvp_name();
        prepare_tvp_param_all();

        SQLRETURN rc;
        auto hstmt = stmt.native_statement_handle();
        *(SQLLEN*)bind_len_or_null->data() = 0 < row_count ? row_count : SQL_DEFAULT_PARAM;

        // bind tvp. tvp_name should Unicode (UTF-16) string
        // https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-table-valued-parameters/binding-and-data-transfer-of-table-valued-parameters-and-column-values
        nanodbc::wide_string tvp_name;
        convert(tvp_name_, tvp_name);

        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            hstmt,
            static_cast<SQLUSMALLINT>(param_index + 1),
            SQL_PARAM_INPUT,
            SQL_C_DEFAULT,
            SQL_SS_TABLE,
            row_count,
            0,
            (SQLPOINTER)tvp_name.data(),
            SQL_NTS,
            bind_len_or_null->data());
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        if (0 == row_count)
        {
            return;
        }

        // set param focus
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLSetStmtAttr),
            rc,
            hstmt,
            SQL_SOPT_SS_PARAM_FOCUS,
            (SQLPOINTER)(std::intptr_t)(param_index + 1),
            SQL_IS_INTEGER);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);
    }

    void close()
    {
        SQLRETURN rc;

        if (open_)
        {
            // if TVP is still open during statement destructor, stmt_ is not valid
            auto stmt_impl = stmt_.lock();

            if (0 < row_count_ && stmt_impl)
            {
                // reset param focus
                NANODBC_CALL_RC(
                    NANODBC_FUNC(SQLSetStmtAttr),
                    rc,
                    stmt_impl->native_statement_handle(),
                    SQL_SOPT_SS_PARAM_FOCUS,
                    (SQLPOINTER) nullptr,
                    SQL_IS_INTEGER);
                if (!success(rc))
                    NANODBC_THROW_DATABASE_ERROR(
                        stmt_impl->native_statement_handle(), SQL_HANDLE_STMT);
            }

            row_count_ = 0;
            tvp_name_.clear();
            param_index_ = 0;
            open_ = false;
            if (stmt_impl)
                stmt_impl->close_tvp();
        }
    }

    void prepare_tvp_name()
    {
        auto stmt_impl = stmt_.lock();
        NANODBC_ASSERT(stmt_impl != nullptr);

        SQLHANDLE hstmt = stmt_impl->native_statement_handle();
        SQLRETURN rc;
        SQLHANDLE hipd;
        SQLINTEGER buf_len, str_len;

        // get ipd handle
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetStmtAttr),
            rc,
            hstmt,
            SQL_ATTR_IMP_PARAM_DESC,
            &hipd,
            SQL_IS_POINTER,
            &str_len);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        // get tvp name buffer length
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetDescField),
            rc,
            hipd,
            param_index_ + 1,
            SQL_CA_SS_TYPE_NAME,
            nullptr,
            0,
            &buf_len);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hipd, SQL_HANDLE_DESC);

        str_len = buf_len / sizeof(nanodbc::string::value_type);
        tvp_name_.resize(str_len);
        buf_len = (str_len + 1) * sizeof(nanodbc::string::value_type);

        // get tvp name
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLGetDescField),
            rc,
            hipd,
            param_index_ + 1,
            SQL_CA_SS_TYPE_NAME,
            &tvp_name_[0],
            buf_len,
            &buf_len);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hipd, SQL_HANDLE_DESC);
    }

    void prepare_tvp_param_all()
    {
        auto stmt_impl = stmt_.lock();
        NANODBC_ASSERT(stmt_impl != nullptr);

        auto stmt = nanodbc::statement(stmt_impl->connection());
        auto hstmt = stmt.native_statement_handle();

        SQLRETURN rc;

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLSetStmtAttr),
            rc,
            hstmt,
            SQL_SOPT_SS_NAME_SCOPE,
            (SQLPOINTER)SQL_SS_NAME_SCOPE_TABLE_TYPE,
            SQL_IS_UINTEGER);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLColumns),
            rc,
            hstmt,
            nullptr,
            0,
            nullptr,
            0,
            (NANODBC_SQLCHAR*)tvp_name_.data(),
            SQL_NTS,
            nullptr,
            0);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        bound_parameter param;
        SQLLEN len[3] = {0};
        param.iotype_ = SQL_PARAM_INPUT;

        NANODBC_CALL_RC(SQLBindCol, rc, hstmt, 5, SQL_C_SSHORT, &param.type_, 0, &len[0]);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        NANODBC_CALL_RC(SQLBindCol, rc, hstmt, 7, SQL_C_SLONG, &param.size_, 0, &len[1]);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        NANODBC_CALL_RC(SQLBindCol, rc, hstmt, 9, SQL_C_SSHORT, &param.scale_, 0, &len[2]);
        if (SQL_SUCCESS != rc && SQL_SUCCESS_WITH_INFO != rc)
            NANODBC_THROW_DATABASE_ERROR(hstmt, SQL_HANDLE_STMT);

        SQLSMALLINT col_no = 0;
        for (;;)
        {
            NANODBC_CALL_RC(SQLFetch, rc, hstmt);
            if (rc != SQL_SUCCESS)
                break;

            param.index_ = col_no;
            param_descr_data_[col_no] = param;
            ++col_no;
        }
    }

    // initializes bind_len_or_null_ and gets information for bind
    void prepare_bind(short param_index, std::size_t batch_size, bound_parameter& param)
    {
        NANODBC_ASSERT(param_index >= 0);

        if (!param_descr_data_.count(param_index))
        {
            throw programming_error("invalid param index");
        }
        else
        {
            param.type_ = param_descr_data_[param_index].type_;
            param.size_ = param_descr_data_[param_index].size_;
            param.scale_ = param_descr_data_[param_index].scale_;
        }

        param.index_ = param_index;
        param.iotype_ = SQL_PARAM_INPUT;

        if (!bind_len_or_null_.count(param_index))
            bind_len_or_null_[param_index] = std::vector<null_type>();
        std::vector<null_type>().swap(bind_len_or_null_[param_index]);

        // ODBC weirdness: this must be at least 8 elements in size
        const std::size_t indicator_size = batch_size > 8 ? batch_size : 8;
        bind_len_or_null_[param_index].reserve(indicator_size);
        bind_len_or_null_[param_index].assign(indicator_size, SQL_NULL_DATA);

        NANODBC_ASSERT(param.index_ == param_index);
        NANODBC_ASSERT(param.iotype_ > 0);
    }

    // calls actual ODBC bind parameter function
    template <class T, typename std::enable_if<!is_character<T>::value, int>::type = 0>
    void bind_parameter(bound_parameter const& param, bound_buffer<T>& buffer)
    {
        NANODBC_ASSERT(buffer.value_size_ > 0 || param.size_ > 0);

        auto value_size{buffer.value_size_};
        if (value_size == 0)
            value_size = param.size_;

        auto param_size{param.size_};
        if (value_size > param_size)
        {
            // Parameter size reported by SQLDescribeParam for Large Objects:
            // - For SQL VARBINARY(MAX), it is Zero which actually means SQL_SS_LENGTH_UNLIMITED.
            // - For SQL UDT (eg. GEOMETRY), it may be driver-specific max limit (eg. SQL Server is
            // DBMAXCHAR=8000 bytes).
            // See MSDN for details
            // https://docs.microsoft.com/en-us/sql/relational-databases/native-client/odbc/large-clr-user-defined-types-odbc
            //
            // If bound value is larger than parameter size, we force SQL_SS_LENGTH_UNLIMITED.
            param_size = SQL_SS_LENGTH_UNLIMITED;
        }

        auto stmt_impl = stmt_.lock();
        NANODBC_ASSERT(stmt_impl != nullptr);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_impl->native_statement_handle(), // handle
            param.index_ + 1,                     // parameter number
            param.iotype_,                        // input or output type
            sql_ctype<T>::value,                  // value type
            param.type_,                          // parameter type
            param_size,   // column size ignored for many types, but needed for strings
            param.scale_, // decimal digits
            (SQLPOINTER)buffer.values_, // parameter value
            value_size,                 // buffer length
            bind_len_or_null_[param.index_].data());

        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_impl->native_statement_handle(), SQL_HANDLE_STMT);
    }

    // Supports code like: query.bind(0, std_string.c_str())
    // In this case, we need to pass nullptr to the final parameter of SQLBindParameter().
    template <class T, typename std::enable_if<is_character<T>::value, int>::type = 0>
    void bind_parameter(bound_parameter const& param, bound_buffer<T>& buffer)
    {
        auto const buffer_size = buffer.value_size_ > 0 ? buffer.value_size_ : param.size_;

        auto stmt_impl = stmt_.lock();
        NANODBC_ASSERT(stmt_impl != nullptr);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_impl->native_statement_handle(), // handle
            param.index_ + 1,                     // parameter number
            param.iotype_,                        // input or output type
            sql_ctype<T>::value,                  // value type
            param.type_,                          // parameter type
            param.size_,  // column size ignored for many types, but needed for strings
            param.scale_, // decimal digits
            (SQLPOINTER)buffer.values_, // parameter value
            buffer_size,                // buffer length
            (buffer.size_ <= 1 ? nullptr : bind_len_or_null_[param.index_].data()));

        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_impl->native_statement_handle(), SQL_HANDLE_STMT);
    }

    template <class T>
    void bind(
        short param_index,
        T const* values,
        std::size_t batch_size,
        bool const* nulls = nullptr,
        T const* null_sentry = nullptr);

    void bind(
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        bool const* nulls = nullptr,
        uint8_t const* null_sentry = nullptr)
    {
        if (values.size() < row_count_)
            throw programming_error("invalid values.size()");

        std::size_t batch_size = row_count_;
        bound_parameter param;
        prepare_bind(param_index, batch_size, param);

        size_t max_length = 0;
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            max_length = std::max(values[i].size(), max_length);
        }
        binary_data_[param_index] = std::vector<uint8_t>(batch_size * max_length, 0);
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            std::copy(
                values[i].begin(),
                values[i].end(),
                binary_data_[param_index].data() + (i * max_length));
        }

        if (null_sentry)
        {
            for (std::size_t i = 0; i < batch_size; ++i)
                if (!std::equal(values[i].begin(), values[i].end(), null_sentry))
                {
                    bind_len_or_null_[param_index][i] = values[i].size();
                }
        }
        else if (nulls)
        {
            for (std::size_t i = 0; i < batch_size; ++i)
            {
                if (!nulls[i])
                    bind_len_or_null_[param_index][i] = values[i].size(); // null terminated
            }
        }
        else
        {
            for (std::size_t i = 0; i < batch_size; ++i)
            {
                bind_len_or_null_[param_index][i] = values[i].size();
            }
        }
        bound_buffer<uint8_t> buffer(binary_data_[param_index].data(), batch_size, max_length);
        bind_parameter(param, buffer);
    }

    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        bool const* nulls = nullptr,
        T const* null_sentry = nullptr);

    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        short param_index,
        std::vector<T> const& values,
        bool const* nulls = nullptr,
        typename T::value_type const* null_sentry = nullptr);

    void bind_null(short param_index)
    {
        bound_parameter param;
        prepare_bind(param_index, row_count_, param);

        auto stmt_impl = stmt_.lock();
        NANODBC_ASSERT(stmt_impl != nullptr);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindParameter,
            rc,
            stmt_impl->native_statement_handle(),
            param.index_ + 1, // parameter number
            param.iotype_,    // input or output typ,
            SQL_C_CHAR,
            param.type_, // parameter type
            param.size_, // column size ignored for many types, but needed for string,
            0,           // decimal digits
            nullptr,     // null value
            0,           // buffer length
            bind_len_or_null_[param.index_].data());
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_impl->native_statement_handle(), SQL_HANDLE_STMT);
    }

    void describe_parameters(
        const std::vector<short>& idx,
        const std::vector<short>& type,
        const std::vector<unsigned long>& size,
        const std::vector<short>& scale)
    {
        if (idx.size() != type.size() || idx.size() != size.size() || idx.size() != scale.size())
            throw programming_error("parameter description arrays are of different size");

        for (std::size_t i = 0; i < idx.size(); ++i)
        {
            param_descr_data_[idx[i]].type_ = static_cast<SQLSMALLINT>(type[i]);
            param_descr_data_[idx[i]].size_ = static_cast<SQLULEN>(size[i]);
            param_descr_data_[idx[i]].scale_ = static_cast<SQLSMALLINT>(scale[i]);
            param_descr_data_[idx[i]].index_ = static_cast<SQLUSMALLINT>(i);
            param_descr_data_[idx[i]].iotype_ = SQL_PARAM_INPUT;
        }
    }

    short parameters() const { return param_descr_data_.size(); }

    unsigned long parameter_size(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<unsigned long>(param_descr_data_.at(param_index).size_);
        }

        prepare_tvp_param_all();
        const SQLULEN& param_size = param_descr_data_.at(param_index).size_;
        NANODBC_ASSERT(
            param_size < static_cast<SQLULEN>(std::numeric_limits<unsigned long>::max()));
        return static_cast<unsigned long>(param_size);
    }

    short parameter_scale(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<short>(param_descr_data_.at(param_index).scale_);
        }

        prepare_tvp_param_all();
        const SQLSMALLINT& param_scale = param_descr_data_.at(param_index).scale_;
        NANODBC_ASSERT(param_scale < static_cast<SQLULEN>(std::numeric_limits<short>::max()));
        return static_cast<short>(param_scale);
    }

    short parameter_type(short param_index)
    {
        if (param_descr_data_.count(param_index))
        {
            return static_cast<short>(param_descr_data_.at(param_index).type_);
        }

        prepare_tvp_param_all();
        const SQLSMALLINT& param_type = param_descr_data_.at(param_index).type_;
        NANODBC_ASSERT(param_type < static_cast<SQLULEN>(std::numeric_limits<short>::max()));
        return static_cast<short>(param_type);
    }

    // comparator for null sentry values
    template <class T>
    bool equals(T const& lhs, T const& rhs)
    {
        return lhs == rhs;
    }

    template <class T>
    std::vector<T>& get_bound_string_data(short param_index);

private:
    std::weak_ptr<nanodbc::statement::statement_impl> stmt_;
    std::size_t row_count_;
    nanodbc::string tvp_name_;
    short param_index_;
    bool open_;
    std::map<short, std::vector<null_type>> bind_len_or_null_;
    std::map<short, std::vector<wide_string::value_type>> wide_string_data_;
    std::map<short, std::vector<std::string::value_type>> string_data_;
    std::map<short, std::vector<uint8_t>> binary_data_;
    std::map<short, bound_parameter> param_descr_data_;
};

template <class T>
void table_valued_parameter::table_valued_parameter_impl::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    bool const* nulls /*= nullptr*/,
    T const* null_sentry /*= nullptr*/)
{
    if (batch_size < row_count_)
        throw programming_error("invalid batch_size");
    batch_size = row_count_;

    bound_parameter param;
    prepare_bind(param_index, batch_size, param);

    if (nulls || null_sentry)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
            if ((null_sentry && !equals(values[i], *null_sentry)) || (nulls && !nulls[i]) || !nulls)
                bind_len_or_null_[param_index][i] = param.size_;
    }
    else
    {
        for (std::size_t i = 0; i < batch_size; ++i)
            bind_len_or_null_[param_index][i] = param.size_;
    }

    bound_buffer<T> buffer(values, batch_size);
    bind_parameter(param, buffer);
}

template <class T, typename>
void table_valued_parameter::table_valued_parameter_impl::bind_strings(
    short param_index,
    std::vector<T> const& values,
    bool const* nulls /*= nullptr*/,
    typename T::value_type const* null_sentry /*= nullptr*/)
{
    if (values.size() < row_count_)
        throw programming_error("invalid values.size()");

    using string_vector = std::vector<typename T::value_type>;
    string_vector& string_data = get_bound_string_data<typename T::value_type>(param_index);

    size_t const batch_size = row_count_;
    bound_parameter param;
    prepare_bind(param_index, batch_size, param);

    size_t max_length = 0;
    for (std::size_t i = 0; i < batch_size; ++i)
    {
        max_length = std::max(values[i].length(), max_length);
    }
    // add space for null terminator
    ++max_length;

    string_data = string_vector(batch_size * max_length, 0);
    for (std::size_t i = 0; i < batch_size; ++i)
    {
        std::copy(values[i].begin(), values[i].end(), string_data.data() + (i * max_length));
    }
    bind_strings(param_index, string_data.data(), max_length, batch_size, nulls, null_sentry);
}

template <class T, typename>
void table_valued_parameter::table_valued_parameter_impl::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    bool const* nulls /*= nullptr*/,
    T const* null_sentry /*= nullptr*/)
{
    if (batch_size < row_count_)
        throw programming_error("invalid batch_size");
    batch_size = row_count_;

    bound_parameter param;
    prepare_bind(param_index, batch_size, param);

    if (null_sentry)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            const std::basic_string<T> s_lhs(
                values + i * value_size, values + (i + 1) * value_size);
            const std::basic_string<T> s_rhs(null_sentry);
            if (!equals(s_lhs, s_rhs))
                bind_len_or_null_[param_index][i] = SQL_NTS;
        }
    }
    else if (nulls)
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            if (!nulls[i])
                bind_len_or_null_[param_index][i] = SQL_NTS; // null terminated
        }
    }
    else
    {
        for (std::size_t i = 0; i < batch_size; ++i)
        {
            bind_len_or_null_[param_index][i] = SQL_NTS;
        }
    }

    auto const buffer_length = value_size * sizeof(T);
    bound_buffer<T> buffer(values, batch_size, buffer_length);
    bind_parameter(param, buffer);
}

template <>
bool table_valued_parameter::table_valued_parameter_impl::equals(
    std::string const& lhs,
    std::string const& rhs)
{
    return std::strncmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
}

template <>
bool table_valued_parameter::table_valued_parameter_impl::equals(
    const wide_string& lhs,
    const wide_string& rhs)
{
    // e6059ff3a79062f83256b9d1d3c9c8368798781e
    // Functions like `swprintf()`, `wcsftime()`, `wcsncmp()` can not be used
    // with `u16string` types. Instead, prefers to narrow unicode string to
    // work with them, and then widen them after work has been completed.
    std::string narrow_lhs;
    narrow_lhs.reserve(lhs.size());
    convert(lhs, narrow_lhs);
    std::string narrow_rhs;
    narrow_rhs.reserve(rhs.size());
    convert(rhs, narrow_rhs);
    return equals(narrow_lhs, narrow_rhs);
}

template <>
bool table_valued_parameter::table_valued_parameter_impl::equals(const date& lhs, const date& rhs)
{
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day;
}

template <>
bool table_valued_parameter::table_valued_parameter_impl::equals(const time& lhs, const time& rhs)
{
    return lhs.hour == rhs.hour && lhs.min == rhs.min && lhs.sec == rhs.sec;
}

template <>
bool table_valued_parameter::table_valued_parameter_impl::equals(
    const timestamp& lhs,
    const timestamp& rhs)
{
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day &&
           lhs.hour == rhs.hour && lhs.min == rhs.min && lhs.sec == rhs.sec &&
           lhs.fract == rhs.fract;
}

template <>
std::vector<wide_string::value_type>&
table_valued_parameter::table_valued_parameter_impl::get_bound_string_data(short param_index)
{
    return wide_string_data_[param_index];
}

template <>
std::vector<std::string::value_type>&
table_valued_parameter::table_valued_parameter_impl::get_bound_string_data(short param_index)
{
    return string_data_[param_index];
}

} // namespace nanodbc

#endif // NANODBC_DISABLE_MSSQL_TVP

// clang-format off
// 8888888b.                            888 888              8888888                        888
// 888   Y88b                           888 888                888                          888
// 888    888                           888 888                888                          888
// 888   d88P .d88b.  .d8888b  888  888 888 888888             888   88888b.d88b.  88888b.  888
// 8888888P" d8P  Y8b 88K      888  888 888 888                888   888 "888 "88b 888 "88b 888
// 888 T88b  88888888 "Y8888b. 888  888 888 888                888   888  888  888 888  888 888
// 888  T88b Y8b.          X88 Y88b 888 888 Y88b.              888   888  888  888 888 d88P 888
// 888   T88b "Y8888   88888P'  "Y88888 888  "Y888           8888888 888  888  888 88888P"  888
//                                                                                 888
//                                                                                 888
//                                                                                 888
// MARK: Result Impl -
// clang-format on

namespace nanodbc
{

class result::result_impl
{
public:
    result_impl(result_impl const&) = delete;
    result_impl& operator=(result_impl const&) = delete;

    result_impl(statement stmt, long rowset_size)
        : stmt_(std::move(stmt))
        , rowset_size_(rowset_size)
        , row_count_(0)
        , bound_columns_(nullptr)
        , bound_columns_size_(0)
        , rowset_position_(0)
        , bound_columns_by_name_()
        , at_end_(false)
#if defined(NANODBC_DO_ASYNC_IMPL)
        , async_(false)
#endif
        /*
         * It is set to true if `unbind` is ever
         * called either by the caller or during the
         * auto_bind process ( blob ).
         * This variable is only used to optimize
         * away unnecessary SQLSetPos calls.
         */
        , has_unbound_(false)
    {
        RETCODE rc;
        NANODBC_CALL_RC(
            SQLSetStmtAttr,
            rc,
            stmt_.native_statement_handle(),
            SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER)(std::intptr_t)rowset_size_,
            0);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

        NANODBC_CALL_RC(
            SQLSetStmtAttr,
            rc,
            stmt_.native_statement_handle(),
            SQL_ATTR_ROWS_FETCHED_PTR,
            &row_count_,
            0);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

        auto_bind_columns();
    }

    ~result_impl() noexcept { cleanup_bound_columns(); }

    void* native_statement_handle() const { return stmt_.native_statement_handle(); }

    long rowset_size() const { return rowset_size_; }

    long affected_rows() const { return stmt_.affected_rows(); }

    bool has_affected_rows() const { return stmt_.affected_rows() != -1; }

    long rows() const noexcept
    {
        NANODBC_ASSERT(row_count_ <= static_cast<SQLULEN>(std::numeric_limits<long>::max()));
        return static_cast<long>(row_count_);
    }

    short columns() const { return stmt_.columns(); }

    bool first()
    {
        rowset_position_ = 0;
        return fetch(0, SQL_FETCH_FIRST);
    }

    bool last()
    {
        rowset_position_ = 0;
        return fetch(0, SQL_FETCH_LAST);
    }

    bool next(void* event_handle = nullptr)
    {
        if (rows() && ++rowset_position_ < rowset_size_)
        {
            if (has_unbound_)
            {
                set_current_position();
            }
            return rowset_position_ < rows();
        }
        rowset_position_ = 0;
        return fetch(0, SQL_FETCH_NEXT, event_handle);
    }

#if defined(NANODBC_DO_ASYNC_IMPL)
    bool async_next(void* event_handle)
    {
        async_ = next(event_handle);
        return async_;
    }

    bool complete_next()
    {
        if (async_)
        {
            RETCODE rc, arc;
            NANODBC_CALL_RC(
                SQLCompleteAsync, rc, SQL_HANDLE_STMT, stmt_.native_statement_handle(), &arc);
            if (arc == SQL_NO_DATA)
            {
                at_end_ = true;
                return false;
            }
            if (!success(rc) || !success(arc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
            async_ = false;
        }
        return !at_end_;
    }
#endif

    bool prior()
    {
        if (rows() && --rowset_position_ >= 0)
        {
            set_current_position();
            return true;
        }
        rowset_position_ = 0;
        return fetch(0, SQL_FETCH_PRIOR);
    }

    bool move(long row)
    {
        rowset_position_ = 0;
        return fetch(row, SQL_FETCH_ABSOLUTE);
    }

    bool skip(long rows)
    {
        rowset_position_ += rows;
        if (this->rows() && rowset_position_ < rowset_size_)
            return rowset_position_ < this->rows();
        rowset_position_ = 0;
        return fetch(rows, SQL_FETCH_RELATIVE);
    }

    unsigned long position() const
    {
        SQLULEN pos = 0; // necessary to initialize to 0
        RETCODE rc;
        NANODBC_CALL_RC(
            SQLGetStmtAttr,
            rc,
            stmt_.native_statement_handle(),
            SQL_ATTR_ROW_NUMBER,
            &pos,
            SQL_IS_UINTEGER,
            nullptr);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

        // MSDN (https://msdn.microsoft.com/en-us/library/ms712631.aspx):
        // If the number of the current row cannot be determined or
        // there is no current row, the driver returns 0.
        // Otherwise, valid row number is returned, starting at 1.
        //
        // NOTE: We try to address incorrect implementation in some drivers (e.g. SQLite ODBC)
        // which instead of 0 return SQL_ROW_NUMBER_UNKNOWN(-2) .
        if (pos == 0 || pos == static_cast<SQLULEN>(SQL_ROW_NUMBER_UNKNOWN))
            return 0;

        NANODBC_ASSERT(pos < static_cast<SQLULEN>(std::numeric_limits<unsigned long>::max()));
        return static_cast<unsigned long>(pos) + rowset_position_;
    }

    bool at_end() const noexcept
    {
        if (at_end_)
            return true;
        SQLULEN pos = 0; // necessary to initialize to 0
        RETCODE rc;
        NANODBC_CALL_RC(
            SQLGetStmtAttr,
            rc,
            stmt_.native_statement_handle(),
            SQL_ATTR_ROW_NUMBER,
            &pos,
            SQL_IS_UINTEGER,
            nullptr);

        // MSDN (https://msdn.microsoft.com/en-us/library/ms712631.aspx):
        // If the number of the current row cannot be determined or
        // there is no current row, the driver returns 0.
        // Otherwise, valid row number is returned, starting at 1.
        //
        // NOTE: We try to address incorrect implementation in some drivers (e.g. SQLite ODBC)
        // which instead of 0 return SQL_ROW_NUMBER_UNKNOWN(-2) .
        if (pos == 0 || pos == static_cast<SQLULEN>(SQL_ROW_NUMBER_UNKNOWN))
            return true;

        SQLULEN const rows_count = rows();
        return (!success(rc) || !(pos < rows_count));
    }

    bool is_null(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        if (rowset_position_ >= rows())
            throw index_range_error();
        return col.cbdata_[static_cast<size_t>(rowset_position_)] == SQL_NULL_DATA;
    }

    bool is_null(string const& column_name) const
    {
        const short column = this->column(column_name);
        return is_null(column);
    }

    bool is_bound(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        return col.bound_;
    }

    bool is_bound(string const& column_name) const
    {
        const short column = this->column(column_name);
        return is_bound(column);
    }

    short column(string const& column_name) const
    {
        auto i = bound_columns_by_name_.find(column_name);
        if (i == bound_columns_by_name_.end())
            throw index_range_error();
        return i->second->column_;
    }

    string column_name(short column) const
    {
        throw_if_column_is_out_of_range(column);
        return bound_columns_[column].name_;
    }

    long column_size(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        NANODBC_ASSERT(col.sqlsize_ <= static_cast<SQLULEN>(std::numeric_limits<long>::max()));
        return static_cast<long>(col.sqlsize_);
    }

    long column_size(string const& column_name) const
    {
        const short column = this->column(column_name);
        return column_size(column);
    }

    int column_decimal_digits(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        return col.scale_;
    }

    int column_decimal_digits(string const& column_name) const
    {
        const short column = this->column(column_name);
        bound_column& col = bound_columns_[column];
        return col.scale_;
    }

    int column_datatype(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        return col.sqltype_;
    }

    int column_datatype(string const& column_name) const
    {
        const short column = this->column(column_name);
        bound_column& col = bound_columns_[column];
        return col.sqltype_;
    }

    string column_datatype_name(short column) const
    {
        throw_if_column_is_out_of_range(column);

        NANODBC_SQLCHAR type_name[256] = {0};
        SQLSMALLINT len = 0; // total number of bytes
        RETCODE rc;
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLColAttribute),
            rc,
            stmt_.native_statement_handle(),
            static_cast<SQLUSMALLINT>(column + 1),
            SQL_DESC_TYPE_NAME,
            type_name,
            sizeof(type_name),
            &len,
            nullptr);
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

        NANODBC_ASSERT(len % sizeof(NANODBC_SQLCHAR) == 0);
        len = len / sizeof(NANODBC_SQLCHAR);
        return {type_name, type_name + len};
    }

    string column_datatype_name(string const& column_name) const
    {
        return column_datatype_name(this->column(column_name));
    }

    int column_c_datatype(short column) const
    {
        throw_if_column_is_out_of_range(column);
        bound_column& col = bound_columns_[column];
        return col.ctype_;
    }

    int column_c_datatype(string const& column_name) const
    {
        const short column = this->column(column_name);
        bound_column& col = bound_columns_[column];
        return col.ctype_;
    }

    bool next_result()
    {
        RETCODE rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
        stmt_.disable_async();
#endif

        NANODBC_CALL_RC(SQLMoreResults, rc, stmt_.native_statement_handle());
        if (rc == SQL_NO_DATA)
            return false;
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        auto_bind_columns();
        return true;
    }

    void unbind()
    {
        short const n_columns = columns();
        if (n_columns < 1)
            return;
        for (short i = 0; i < n_columns; ++i)
            unbind(i);
    }

    void unbind(short column)
    {
        throw_if_column_is_out_of_range(column);
        has_unbound_ = true;
        if (is_bound(column))
        {
            bound_column& col = bound_columns_[column];
            unbind_column(col);
        }
    }

    void unbind(string const& column_name)
    {
        short const column = this->column(column_name);
        unbind(column);
    }

    template <
        class T
#ifdef NANODBC_HAS_STD_OPTIONAL
        ,
        std::enable_if_t<!is_optional<T>::value, int> = 0
#endif
        >
    void get_ref(short column, T& result) const
    {
        throw_if_column_is_out_of_range(column);
        if (is_null(column))
            throw null_access_error();
        get_ref_impl<T>(column, result);
    }

#ifdef NANODBC_HAS_STD_OPTIONAL
    template <class T, std::enable_if_t<is_optional<T>::value, int> = 0>
    void get_ref(short column, T& result) const
    {
        throw_if_column_is_out_of_range(column);
        if (is_null(column))
        {
            opt_reset(result);
            return;
        }
        get_ref_impl<std::remove_reference_t<decltype(*result)>>(column, *result);
    }
#endif

    template <class T>
    void get_ref(short column, T const& fallback, T& result) const
    {
        throw_if_column_is_out_of_range(column);
        if (is_null(column))
        {
            // MySQL: Bug or non-standard behaviour? SQLBindCol sets the indicator to SQL_NULL_DATA
            // NANODBC_ASSERT(is_bound(column));
            result = fallback;
            return;
        }
        get_ref_impl<T>(column, result);

        // for unbound columns, null indicator is determined by SQLGetData call
        if (is_null(column))
        {
            NANODBC_ASSERT(!is_bound(column));
            result = fallback;
        }
    }

    template <
        class T
#ifdef NANODBC_HAS_STD_OPTIONAL
        ,
        std::enable_if_t<!is_optional<T>::value, int> = 0
#endif
        >
    void get_ref(string const& column_name, T& result) const
    {
        short const column = this->column(column_name);
        if (is_null(column))
            throw null_access_error();
        get_ref_impl<T>(column, result);
    }

#ifdef NANODBC_HAS_STD_OPTIONAL
    template <class T, std::enable_if_t<is_optional<T>::value, int> = 0>
    void get_ref(string const& column_name, T& result) const
    {
        short const column = this->column(column_name);
        if (is_null(column))
        {
            opt_reset(result);
            return;
        }
        get_ref_impl<std::remove_reference_t<decltype(*result)>>(column, *result);
    }
#endif

    template <class T>
    void get_ref(string const& column_name, T const& fallback, T& result) const
    {
        short const column = this->column(column_name);
        if (is_null(column))
        {
            NANODBC_ASSERT(is_bound(column));
            result = fallback;
            return;
        }
        get_ref_impl<T>(column, result);

        // for unbound columns, null indicator is determined by SQLGetData call
        if (is_null(column))
        {
            NANODBC_ASSERT(!is_bound(column));
            result = fallback;
        }
    }

    template <class T>
    T get(short column) const
    {
        T result;
        get_ref(column, result);
        return result;
    }

    template <class T>
    T get(short column, T const& fallback) const
    {
        T result;
        get_ref(column, fallback, result);
        return result;
    }

    template <class T>
    T get(string const& column_name) const
    {
        T result;
        get_ref(column_name, result);
        return result;
    }

    template <class T>
    T get(string const& column_name, T const& fallback) const
    {
        T result;
        get_ref(column_name, fallback, result);
        return result;
    }

private:
    template <typename T>
    std::unique_ptr<T, std::function<void(T*)>> ensure_pdata(short column) const;

    template <class T, typename std::enable_if<!is_string<T>::value, int>::type = 0>
    void get_ref_impl(short column, T& result) const;

    template <class T, typename std::enable_if<is_string<T>::value, int>::type = 0>
    void get_ref_impl(short column, T& result) const;

    template <class T, typename std::enable_if<!is_character<T>::value, int>::type = 0>
    void get_ref_from_string_column(short column, T& result) const;

    template <class T, typename std::enable_if<is_character<T>::value, int>::type = 0>
    void get_ref_from_string_column(short column, T& result) const;

    void throw_if_column_is_out_of_range(short column) const
    {
        if ((column < 0) || (column >= bound_columns_size_))
            throw index_range_error();
    }

    void before_move() noexcept
    {
        for (short i = 0; i < bound_columns_size_; ++i)
        {
            bound_column& col = bound_columns_[i];
            for (std::size_t j = 0; j < static_cast<size_t>(rowset_size_); ++j)
                col.cbdata_[j] = 0;
            if (col.blob_ && col.pdata_)
                release_bound_resources(i);
        }
    }

    void release_bound_resources(short column) noexcept
    {
        NANODBC_ASSERT(column < bound_columns_size_);
        bound_column& col = bound_columns_[column];
        delete[] col.pdata_;
        col.pdata_ = nullptr;
        col.clen_ = 0;
    }

    void cleanup_bound_columns() noexcept
    {
        before_move();
        delete[] bound_columns_;
        bound_columns_ = nullptr;
        bound_columns_size_ = 0;
        bound_columns_by_name_.clear();
    }

    // If event_handle is specified, fetch returns true iff the statement is still executing
    bool fetch(long rows, SQLUSMALLINT orientation, void* event_handle = nullptr)
    {
        before_move();

#if defined(NANODBC_DO_ASYNC_IMPL)
        if (event_handle == nullptr)
            stmt_.disable_async();
        else
            stmt_.enable_async(event_handle);
#endif // !NANODBC_DISABLE_ASYNC && SQL_ATTR_ASYNC_STMT_EVENT && SQL_API_SQLCOMPLETEASYNC

        RETCODE rc;
        NANODBC_CALL_RC(SQLFetchScroll, rc, stmt_.native_statement_handle(), orientation, rows);
        if (rc == SQL_NO_DATA)
        {
            at_end_ = true;
            return false;
        }
#if defined(NANODBC_DO_ASYNC_IMPL)
        if (event_handle != nullptr)
            return rc == SQL_STILL_EXECUTING;
#endif
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        return true;
    }

    void auto_bind_columns()
    {
        cleanup_bound_columns();

        const short n_columns = columns();
        if (n_columns < 1)
            return;

        NANODBC_ASSERT(!bound_columns_);
        NANODBC_ASSERT(!bound_columns_size_);
        bound_columns_ = new bound_column[n_columns];
        bound_columns_size_ = n_columns;

        RETCODE rc;
        NANODBC_SQLCHAR column_name[1024];
        SQLSMALLINT sqltype = 0, scale = 0, nullable = 0, len = 0;
        SQLULEN sqlsize = 0;

#if defined(NANODBC_DO_ASYNC_IMPL)
        stmt_.disable_async();
#endif

        for (SQLSMALLINT i = 0; i < n_columns; ++i)
        {
            NANODBC_CALL_RC(
                NANODBC_FUNC(SQLDescribeCol),
                rc,
                stmt_.native_statement_handle(),
                static_cast<SQLUSMALLINT>(i + 1),
                (NANODBC_SQLCHAR*)column_name,
                sizeof(column_name) / sizeof(NANODBC_SQLCHAR),
                &len,
                &sqltype,
                &sqlsize,
                &scale,
                &nullable);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

            bound_column& col = bound_columns_[i];
            col.name_ = reinterpret_cast<string::value_type*>(column_name);
            col.column_ = i;
            col.sqltype_ = sqltype;
            col.sqlsize_ = sqlsize;
            col.scale_ = scale;
            bound_columns_by_name_[col.name_] = &col;

            using namespace std; // if int64_t is in std namespace (in c++11)
            switch (col.sqltype_)
            {
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_BIGINT:
                col.ctype_ = SQL_C_SBIGINT;
                col.clen_ = sizeof(int64_t);
                break;
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
                col.ctype_ = SQL_C_DOUBLE;
                col.clen_ = sizeof(double);
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
                col.ctype_ = SQL_C_CHAR;
                // SQL column size defines number of digits without the decimal mark
                // and without minus sign which may also occur.
                // We need to adjust buffer length allow space for null-termination character
                // as well as the fractional part separator and the minus sign.
                col.clen_ = (col.sqlsize_ + 1 + 1 + 1) * sizeof(SQLCHAR);
                break;
            case SQL_DATE:
            case SQL_TYPE_DATE:
                col.ctype_ = SQL_C_DATE;
                col.clen_ = sizeof(date);
                break;
            case SQL_TIME:
            case SQL_TYPE_TIME:
            case SQL_SS_TIME2:
                col.ctype_ = SQL_C_TIME;
                col.clen_ = sizeof(time);
                break;
            case SQL_TIMESTAMP:
            case SQL_TYPE_TIMESTAMP:
                col.ctype_ = SQL_C_TIMESTAMP;
                col.clen_ = sizeof(timestamp);
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
                col.ctype_ = sql_ctype<std::string>::value;
                col.clen_ = NBYTES(col.sqlsize_, SQLCHAR);
                if (col.sqlsize_ == 0)
                {
                    col.clen_ = 0;
                    col.blob_ = true;
                }
                break;
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_SS_XML:
                col.ctype_ = sql_ctype<wide_string>::value;
                col.clen_ = NBYTES(col.sqlsize_, SQLWCHAR);
                if (col.sqlsize_ == 0)
                {
                    col.clen_ = 0;
                    col.blob_ = true;
                }
                break;
            case SQL_SS_TIMESTAMPOFFSET:
                col.ctype_ = sql_ctype<wide_string>::value;
                col.clen_ = (col.sqlsize_ + 1) * sizeof(SQLWCHAR);
                break;
            case SQL_LONGVARCHAR:
                col.ctype_ = sql_ctype<std::string>::value;
                col.blob_ = true;
                col.clen_ = 0;
                break;
            case SQL_WLONGVARCHAR:
                col.ctype_ = sql_ctype<wide_string>::value;
                col.blob_ = true;
                col.clen_ = 0;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
            case SQL_SS_UDT: // MSDN: Essentially, UDT is a varbinary type with additional metadata.
                col.ctype_ = SQL_C_BINARY;
                col.blob_ = true;
                col.clen_ = 0;
                break;
            default:
                col.ctype_ = sql_ctype<string>::value;
                col.clen_ = 128;
                break;
            }
        }

        for (SQLSMALLINT i = 0; i < n_columns; ++i)
        {
            bound_column& col = bound_columns_[i];
            col.cbdata_ = new null_type[static_cast<size_t>(rowset_size_)];
            if (col.blob_)
            {
                unbind_column(col);
            }
            else
            {
                col.pdata_ = new char[rowset_size_ * col.clen_];
                bind_column(col);
            }
        }
    }

    void bind_column(bound_column& column)
    {
        NANODBC_ASSERT(column.pdata_);
        NANODBC_ASSERT(column.cbdata_);

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindCol,
            rc,
            stmt_.native_statement_handle(),
            static_cast<SQLUSMALLINT>(column.column_ + 1), // ColumnNumber
            column.ctype_,                                 // TargetType
            column.pdata_,                                 // TargetValuePtr
            column.clen_,                                  // BufferLength
            column.cbdata_);                               // StrLen_or_Ind
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        column.bound_ = true;
    }

    void unbind_column(bound_column& column)
    {
        NANODBC_ASSERT(column.cbdata_);
        has_unbound_ = true;

        RETCODE rc;
        NANODBC_CALL_RC(
            SQLBindCol,
            rc,
            stmt_.native_statement_handle(),
            static_cast<SQLUSMALLINT>(column.column_ + 1), // ColumnNumber
            column.ctype_,
            nullptr,
            0,
            column.cbdata_); // re-use existing cbdata_ buffer
        if (!success(rc))
            NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        delete[] column.pdata_;
        column.pdata_ = nullptr;
        column.bound_ = false;
    }

    void set_current_position()
    {
        if (rowset_position_ < rowset_size_ && rowset_position_ < rows())
        {
            RETCODE rc;
            NANODBC_CALL_RC(
                SQLSetPos,
                rc,
                stmt_.native_statement_handle(),
                static_cast<SQLSETPOSIROW>(rowset_position_) + 1,
                SQL_POSITION,
                SQL_LOCK_NO_CHANGE);
            if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        }
    }

private:
    statement stmt_;
    const long rowset_size_;
    SQLULEN row_count_;
    bound_column* bound_columns_;
    short bound_columns_size_;
    long rowset_position_;
    std::map<string, bound_column*> bound_columns_by_name_;
    bool at_end_;
#if defined(NANODBC_DO_ASYNC_IMPL)
    bool async_; // true if statement is currently in SQL_STILL_EXECUTING mode
#endif
    bool has_unbound_;
};

template <>
inline void result::result_impl::get_ref_impl<date>(short column, date& result) const
{
    bound_column& col = bound_columns_[column];
    switch (col.ctype_)
    {
    case SQL_C_DATE:
        result = *ensure_pdata<date>(column);
        return;
    case SQL_C_TIMESTAMP:
    {
        timestamp stamp = *ensure_pdata<timestamp>(column);
        date d = {stamp.year, stamp.month, stamp.day};
        result = d;
        return;
    }
    }
    throw type_incompatible_error();
}

template <>
inline void result::result_impl::get_ref_impl<time>(short column, time& result) const
{
    bound_column& col = bound_columns_[column];
    switch (col.ctype_)
    {
    case SQL_C_TIME:
        result = *ensure_pdata<time>(column);
        return;
    case SQL_C_TIMESTAMP:
    {
        timestamp stamp = *ensure_pdata<timestamp>(column);
        time t = {stamp.hour, stamp.min, stamp.sec};
        result = t;
        return;
    }
    }
    throw type_incompatible_error();
}

template <>
inline void result::result_impl::get_ref_impl<timestamp>(short column, timestamp& result) const
{
    bound_column& col = bound_columns_[column];
    switch (col.ctype_)
    {
    case SQL_C_DATE:
    {
        date d = *ensure_pdata<date>(column);
        timestamp stamp = {d.year, d.month, d.day, 0, 0, 0, 0};
        result = stamp;
        return;
    }
    case SQL_C_TIMESTAMP:
        result = *ensure_pdata<timestamp>(column);
        return;
    }
    throw type_incompatible_error();
}

template <class T, typename std::enable_if<is_string<T>::value, int>::type>
inline void result::result_impl::get_ref_impl(short column, T& result) const
{
    bound_column& col = bound_columns_[column];
    const SQLULEN column_size = col.sqlsize_;

    switch (col.ctype_)
    {
    case SQL_C_CHAR:
    case SQL_C_BINARY:
    {
        if (!is_bound(column))
        {
            // Input is always std::string, while output may be std::string or wide_string
            std::string out;
            // The length of the data available to return, decreasing with subsequent SQLGetData
            // calls.
            // But, NOT the length of data returned into the buffer (apart from the final call).
            SQLLEN ValueLenOrInd;
            SQLRETURN rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
            stmt_.disable_async();
#endif

            void* handle = native_statement_handle();
            do
            {
                char buffer[1024] = {0};
                const std::size_t buffer_size = sizeof(buffer);
                NANODBC_CALL_RC(
                    SQLGetData,
                    rc,
                    handle,                                // StatementHandle
                    static_cast<SQLUSMALLINT>(column + 1), // Col_or_Param_Num
                    col.ctype_,                            // TargetType
                    buffer,                                // TargetValuePtr
                    buffer_size,                           // BufferLength
                    &ValueLenOrInd);                       // StrLen_or_IndPtr
                if (ValueLenOrInd == SQL_NO_TOTAL)
                    out.append(buffer, col.ctype_ == SQL_C_BINARY ? buffer_size : buffer_size - 1);
                else if (ValueLenOrInd > 0)
                    out.append(
                        buffer,
                        std::min<std::size_t>(
                            ValueLenOrInd,
                            col.ctype_ == SQL_C_BINARY ? buffer_size : buffer_size - 1));
                else if (ValueLenOrInd == SQL_NULL_DATA)
                    col.cbdata_[static_cast<size_t>(rowset_position_)] = (SQLINTEGER)SQL_NULL_DATA;
                // Sequence of successful calls is:
                // SQL_NO_DATA or SQL_SUCCESS_WITH_INFO followed by SQL_SUCCESS.
            } while (rc == SQL_SUCCESS_WITH_INFO);
            if (rc == SQL_SUCCESS || rc == SQL_NO_DATA)
                convert(std::move(out), result);
            else if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        }
        else
        { // bound and not blob
            const char* s = col.pdata_ + rowset_position_ * col.clen_;
            convert(s, result);
        }
        return;
    }

    case SQL_C_WCHAR:
    {
        if (!is_bound(column))
        {
            // Input is always wide_string, output might be std::string or wide_string.
            // Use a string builder to build the output string.
            wide_string out;
            // The length of the data available to return, decreasing with subsequent SQLGetData
            // calls.
            // But, NOT the length of data returned into the buffer (apart from the final call).
            SQLLEN ValueLenOrInd;
            SQLRETURN rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
            stmt_.disable_async();
#endif

            void* handle = native_statement_handle();
            do
            {
                wide_char_t buffer[512] = {0};
                const std::size_t buffer_size = sizeof(buffer);
                NANODBC_CALL_RC(
                    SQLGetData,
                    rc,
                    handle,                                // StatementHandle
                    static_cast<SQLUSMALLINT>(column + 1), // Col_or_Param_Num
                    col.ctype_,                            // TargetType
                    buffer,                                // TargetValuePtr
                    buffer_size,                           // BufferLength
                    &ValueLenOrInd);                       // StrLen_or_IndPtr
                if (ValueLenOrInd == SQL_NO_TOTAL)
                    out.append(buffer, (buffer_size / sizeof(wide_char_t)) - 1);
                else if (ValueLenOrInd > 0)
                    out.append(
                        buffer,
                        std::min<std::size_t>(
                            ValueLenOrInd / sizeof(wide_char_t),
                            (buffer_size / sizeof(wide_char_t)) - 1));
                else if (ValueLenOrInd == SQL_NULL_DATA)
                    col.cbdata_[static_cast<std::size_t>(rowset_position_)] =
                        (SQLINTEGER)SQL_NULL_DATA;
                // Sequence of successful calls is:
                // SQL_NO_DATA or SQL_SUCCESS_WITH_INFO followed by SQL_SUCCESS.
            } while (rc == SQL_SUCCESS_WITH_INFO);
            if (rc == SQL_SUCCESS || rc == SQL_NO_DATA)
                convert(std::move(out), result);
            else if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        }
        else
        { // bound and not blob
            SQLWCHAR const* s =
                reinterpret_cast<SQLWCHAR*>(col.pdata_ + rowset_position_ * col.clen_);
            string::size_type const str_size =
                col.cbdata_[static_cast<size_t>(rowset_position_)] / sizeof(SQLWCHAR);
            auto const us = reinterpret_cast<wide_char_t const*>(
                s); // no-op or unsigned short to signed char16_t
            convert(us, str_size, result);
        }
        return;
    }

    case SQL_C_LONG:
    {
        std::string buffer(column_size + 1, 0); // ensure terminating null
        const int32_t data = *ensure_pdata<int32_t>(column);
        const int bytes =
            std::snprintf(const_cast<char*>(buffer.data()), column_size + 1, "%d", data);
        if (bytes == -1)
            throw type_incompatible_error();
        convert(buffer.data(), result); // passing the C pointer drops trailing nulls
        return;
    }

    case SQL_C_SBIGINT:
    {
        using namespace std;                    // in case intmax_t is in namespace std
        std::string buffer(column_size + 1, 0); // ensure terminating null
        const intmax_t data = (intmax_t)*ensure_pdata<int64_t>(column);
        const int bytes =
            std::snprintf(const_cast<char*>(buffer.data()), column_size + 1, "%jd", data);
        if (bytes == -1)
            throw type_incompatible_error();
        convert(buffer.data(), result); // passing the C pointer drops trailing nulls
        return;
    }

    case SQL_C_FLOAT:
    {
        std::string buffer(column_size + 1, 0); // ensure terminating null
        const float data = *ensure_pdata<float>(column);
        const int bytes =
            std::snprintf(const_cast<char*>(buffer.data()), column_size + 1, "%f", data);
        if (bytes == -1)
            throw type_incompatible_error();
        convert(buffer.data(), result); // passing the C pointer drops trailing nulls
        return;
    }

    case SQL_C_DOUBLE:
    {
        const SQLULEN width = column_size + 2; // account for decimal mark and sign
        std::string buffer(width + 1, 0);      // ensure terminating null
        const double data = *ensure_pdata<double>(column);
        const int bytes = std::snprintf(
            const_cast<char*>(buffer.data()),
            width + 1,
            "%f", // do not restrict the number of digits, because for floating-point
                  // columns the scale is undefined - number of digits to the right
                  // of the decimal point is not fixed
            data);
        if (bytes == -1)
            throw type_incompatible_error();
        convert(buffer.data(), result); // passing the C pointer drops trailing nulls
        return;
    }

    case SQL_C_DATE:
    {
        const date d = *ensure_pdata<date>(column);
        std::tm st{};
        st.tm_year = d.year - 1900;
        st.tm_mon = d.month - 1;
        st.tm_mday = d.day;
        std::string old_lc_time_container;
        const char* old_lc_time = nullptr;
        if (char* olc_lc_time_ptr = std::setlocale(LC_TIME, nullptr))
        {
            old_lc_time_container = olc_lc_time_ptr;
            old_lc_time = old_lc_time_container.c_str();
        }
        std::setlocale(LC_TIME, "");
        char date_str[512];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", &st);
        std::setlocale(LC_TIME, old_lc_time);
        convert(date_str, result);
        return;
    }

    case SQL_C_TIME:
    {
        const time t = *ensure_pdata<time>(column);
        std::tm st{};
        st.tm_hour = t.hour;
        st.tm_min = t.min;
        st.tm_sec = t.sec;
        std::string old_lc_time_container;
        const char* old_lc_time = nullptr;
        if (char* olc_lc_time_ptr = std::setlocale(LC_TIME, nullptr))
        {
            old_lc_time_container = olc_lc_time_ptr;
            old_lc_time = old_lc_time_container.c_str();
        }
        std::setlocale(LC_TIME, "");
        char date_str[512];
        std::strftime(date_str, sizeof(date_str), "%H:%M:%S", &st);
        std::setlocale(LC_TIME, old_lc_time);
        convert(date_str, result);
        return;
    }

    case SQL_C_TIMESTAMP:
    {
        const timestamp stamp = *ensure_pdata<timestamp>(column);
        std::tm st{};
        st.tm_year = stamp.year - 1900;
        st.tm_mon = stamp.month - 1;
        st.tm_mday = stamp.day;
        st.tm_hour = stamp.hour;
        st.tm_min = stamp.min;
        st.tm_sec = stamp.sec;
        std::string old_lc_time_container;
        const char* old_lc_time = nullptr;
        if (char* olc_lc_time_ptr = std::setlocale(LC_TIME, nullptr))
        {
            old_lc_time_container = olc_lc_time_ptr;
            old_lc_time = old_lc_time_container.c_str();
        }
        std::setlocale(LC_TIME, "");
        char date_str[512];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S %z", &st);
        std::setlocale(LC_TIME, old_lc_time);
        convert(date_str, result);
        return;
    }
    }
    throw type_incompatible_error();
}

template <>
inline void result::result_impl::get_ref_impl<std::vector<std::uint8_t>>(
    short column,
    std::vector<std::uint8_t>& result) const
{
    bound_column& col = bound_columns_[column];
    const SQLULEN column_size = col.sqlsize_;

    switch (col.ctype_)
    {
    case SQL_C_BINARY:
    {
        if (!is_bound(column))
        {
            // Input and output is always array of bytes.
            std::vector<std::uint8_t> out;
            std::uint8_t buffer[1024] = {0};
            std::size_t const buffer_size = sizeof(buffer);
            // The length of the data available to return, decreasing with subsequent SQLGetData
            // calls.
            // But, NOT the length of data returned into the buffer (apart from the final call).
            SQLLEN ValueLenOrInd;
            SQLRETURN rc;

#if defined(NANODBC_DO_ASYNC_IMPL)
            stmt_.disable_async();
#endif

            void* handle = native_statement_handle();
            do
            {
                NANODBC_CALL_RC(
                    SQLGetData,
                    rc,
                    handle,                                // StatementHandle
                    static_cast<SQLUSMALLINT>(column + 1), // Col_or_Param_Num
                    SQL_C_BINARY,                          // TargetType
                    buffer,                                // TargetValuePtr
                    buffer_size,                           // BufferLength
                    &ValueLenOrInd);                       // StrLen_or_IndPtr
                if (ValueLenOrInd > 0)
                {
                    auto const buffer_size_filled =
                        std::min<std::size_t>(ValueLenOrInd, buffer_size);
                    NANODBC_ASSERT(buffer_size_filled <= buffer_size);
                    out.insert(std::end(out), buffer, buffer + buffer_size_filled);
                }
                else if (ValueLenOrInd == SQL_NULL_DATA)
                    col.cbdata_[static_cast<size_t>(rowset_position_)] = (SQLINTEGER)SQL_NULL_DATA;
                // Sequence of successful calls is:
                // SQL_NO_DATA or SQL_SUCCESS_WITH_INFO followed by SQL_SUCCESS.
            } while (rc == SQL_SUCCESS_WITH_INFO);
            if (rc == SQL_SUCCESS || rc == SQL_NO_DATA)
                result = std::move(out);
            else if (!success(rc))
                NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);
        }
        else
        {
            // Read fixed-length binary data
            const char* s = col.pdata_ + rowset_position_ * col.clen_;
            result.assign(s, s + column_size);
        }
        return;
    }
    }
    throw type_incompatible_error();
}

#if defined(_MSC_VER)
template <>
inline void result::result_impl::get_ref_impl<_variant_t>(short column, _variant_t& result) const
{
    result.Clear(); // VT_EMPTY, not SQL-like VT_NULL
    bound_column& col = bound_columns_[column];
    auto c_type = col.ctype_;

    // SQL type to C type mapping could have been simplified by auto-binding
    // FIXME: Correct the auto_bind_columns to not to 'flatten' types
    auto const sql_type = col.sqltype_;
    switch (sql_type)
    {
    case SQL_BIT:
        c_type = SQL_C_BIT;
        break;
    case SQL_DECIMAL:
    case SQL_NUMERIC:
        c_type = SQL_C_NUMERIC;
        break;
    }

    switch (c_type)
    {
    case SQL_C_BINARY:
    {
        // TODO: Optimise with bespoke implementation of get_ref_impl<SAFEARRAY>
        std::vector<std::uint8_t> v;
        get_ref_impl(column, v);
        ::SAFEARRAYBOUND bounds[1] = {static_cast<unsigned long>(v.size()), 0};
        result.vt = VT_ARRAY | VT_UI1;
        result.parray = ::SafeArrayCreate(VT_UI1, 1, bounds);
        if (!v.empty())
        {
            void* data = nullptr;
            ::SafeArrayAccessData(result.parray, &data);
            std::memcpy(data, &v[0], v.size());
            ::SafeArrayUnaccessData(result.parray);
        }
        else
        {
            // Work around VariantChangeType limitation for VT_ARRAY (called in case of NULL, see
            // below)
            result.Clear();
        }
        break;
    }
    case SQL_C_BIT:
    {
        auto const v = *(ensure_pdata<short>(column));
        result = _variant_t(!!v ? VARIANT_TRUE : VARIANT_FALSE, VT_BOOL);
        break;
    }
    case SQL_C_CHAR:
    {
        std::string v;
        get_ref_impl(column, v);
        result = v.c_str();
        break;
    }
    case SQL_C_WCHAR:
    {
        std::wstring v;
        get_ref_impl(column, v);
        result = v.c_str();
        break;
    }
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
        result = (char)*(ensure_pdata<short>(column));
        break;
    case SQL_C_UTINYINT:
        result = (unsigned char)*(ensure_pdata<unsigned short>(column));
        break;
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
        result = *(ensure_pdata<short>(column));
        break;
    case SQL_C_USHORT:
        result = *(ensure_pdata<unsigned short>(column));
        break;
    case SQL_C_LONG:
    case SQL_C_SLONG:
        result = *(ensure_pdata<int32_t>(column));
        break;
    case SQL_C_ULONG:
        result = *(ensure_pdata<uint32_t>(column));
        break;
    case SQL_C_SBIGINT:
        result = *(ensure_pdata<int64_t>(column));
        break;
    case SQL_C_UBIGINT:
        result = *(ensure_pdata<uint64_t>(column));
        break;
    case SQL_C_DOUBLE:
        result = *(ensure_pdata<double>(column));
        break;
    case SQL_C_FLOAT:
        result = *(ensure_pdata<float>(column));
        break;
    case SQL_C_NUMERIC:
    {
        // FIXME: Likely, this is never called as SQL_DECIMAL is auto-bound as SQL_C_CHAR
        // TODO: Review this for SQL Server (and other databases?) types money, smallmoney as VT_CY
        std::wstring v;
        get_ref_impl(column, v);
        DECIMAL d{0};
        if (!v.empty())
        {
#ifdef __MINGW32__
            //  See https://sourceforge.net/p/mingw-w64/bugs/940/
            auto s = const_cast<LPOLESTR>(static_cast<LPCOLESTR>(v.c_str()));
#else
            auto s = static_cast<LPCOLESTR>(v.c_str());
#endif
            if (FAILED(::VarDecFromStr(s, LOCALE_INVARIANT, 0, &d)))
                throw type_incompatible_error();
        }
        result = d;
        break;
    }
    case SQL_C_DATE:
    case SQL_C_TYPE_DATE:
    {
        nanodbc::date v{0};
        get_ref_impl(column, v);
        ::SYSTEMTIME st{
            static_cast<WORD>(v.year),
            static_cast<WORD>(v.month),
            0,
            static_cast<WORD>(v.day),
            0,
            0,
            0,
            0};
        ::DATE date{0};
        if (!::SystemTimeToVariantTime(&st, &date))
            throw type_incompatible_error();
        result = _variant_t(date, VT_DATE);
        break;
    }
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
    {
        nanodbc::time v{0};
        get_ref_impl(column, v);
        ::SYSTEMTIME st{
            0,
            0,
            0,
            0,
            static_cast<WORD>(v.hour),
            static_cast<WORD>(v.min),
            static_cast<WORD>(v.sec),
            0};
        ::DATE date{0};
        if (!::SystemTimeToVariantTime(&st, &date))
            throw type_incompatible_error();
        result = _variant_t(date, VT_DATE);
        break;
    }
    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP:
    {
        nanodbc::timestamp v{0};
        get_ref_impl(column, v);
        SYSTEMTIME st{
            static_cast<WORD>(v.year),
            static_cast<WORD>(v.month),
            0,
            static_cast<WORD>(v.day),
            static_cast<WORD>(v.hour),
            static_cast<WORD>(v.min),
            static_cast<WORD>(v.sec),
            static_cast<unsigned short>(v.fract)};
        DATE date;
        if (!::SystemTimeToVariantTime(&st, &date))
            throw type_incompatible_error();
        result = _variant_t(date, VT_DATE);
        break;
    }
    default:
        std::wstring v;
        get_ref_impl(column, v);
        result = v.c_str();
    }

    if (is_null(column))
        result.ChangeType(VT_NULL);
}
#endif

namespace detail
{
auto from_string(std::string const& s, float)
{
    return std::stof(s);
}

auto from_string(std::string const& s, double)
{
    return std::stod(s);
}

auto from_string(std::string const& s, long long)
{
    return std::stoll(s);
}

auto from_string(std::string const& s, unsigned long long)
{
    return std::stoull(s);
}

template <typename R, typename std::enable_if<std::is_integral<R>::value, int>::type = 0>
auto from_string(std::string const& s, R)
{
    auto integer = from_string(
        s,
        typename std::conditional<std::is_signed<R>::value, long long, unsigned long long>::type{});
    if (integer > std::numeric_limits<R>::max() || integer < std::numeric_limits<R>::min())
        throw std::range_error("from_string argument out of range");
    return static_cast<R>(integer);
}

#if defined(_MSC_VER)
auto from_string(std::string const& s, _variant_t)
{
    return s.c_str();
}
#endif

} // namespace detail

template <typename R>
auto from_string(std::string const& s) -> R
{
    return detail::from_string(s, R{});
}

template <class T, typename std::enable_if<is_character<T>::value, int>::type>
void result::result_impl::get_ref_from_string_column(short column, T& result) const
{
    bound_column& col = bound_columns_[column];
    switch (col.ctype_)
    {
    case SQL_C_CHAR:
        result = static_cast<T>(*ensure_pdata<char>(column));
        return;
    case SQL_C_WCHAR:
        result = static_cast<T>(*ensure_pdata<wide_char_t>(column));
        return;
    }
    throw type_incompatible_error();
}

template <class T, typename std::enable_if<!is_character<T>::value, int>::type>
void result::result_impl::get_ref_from_string_column(short column, T& result) const
{
    bound_column& col = bound_columns_[column];
    if (col.ctype_ != SQL_C_CHAR && col.ctype_ != SQL_C_WCHAR)
        throw type_incompatible_error();
    std::string str;
    get_ref_impl(col.column_, str);
    result = from_string<T>(str);
}

template <typename T>
std::unique_ptr<T, std::function<void(T*)>> result::result_impl::ensure_pdata(short column) const
{
    bound_column& col = bound_columns_[column];
    SQLLEN ValueLenOrInd;
    SQLRETURN rc;
    if (is_bound(column))
    {
        // Return a unique_ptr with a no-op deleter as this memory allocation
        // is managed (allocated and released) elsewhere.
        return std::unique_ptr<T, std::function<void(T*)>>(
            (T*)(col.pdata_ + rowset_position_ * col.clen_), [](T*) {});
    }

    std::unique_ptr<T> buffer = std::make_unique<T>();
    const std::size_t buffer_size = sizeof(T);
    void* handle = native_statement_handle();
    NANODBC_CALL_RC(
        SQLGetData,
        rc,
        handle,                                // StatementHandle
        static_cast<SQLUSMALLINT>(column + 1), // Col_or_Param_Num
        sql_ctype<T>::value,                   // TargetType
        buffer.get(),                          // TargetValuePtr
        buffer_size,                           // BufferLength
        &ValueLenOrInd);                       // StrLen_or_IndPtr

    if (ValueLenOrInd == SQL_NULL_DATA)
        col.cbdata_[static_cast<size_t>(rowset_position_)] = (SQLINTEGER)SQL_NULL_DATA;

    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt_.native_statement_handle(), SQL_HANDLE_STMT);

    return buffer;
}

template <class T, typename std::enable_if<!is_string<T>::value, int>::type>
void result::result_impl::get_ref_impl(short column, T& result) const
{
    bound_column& col = bound_columns_[column];
    using namespace std; // if int64_t is in std namespace (in c++11)
    switch (col.ctype_)
    {
    case SQL_C_CHAR:
    case SQL_C_WCHAR:
        get_ref_from_string_column(column, result);
        return;
    case SQL_C_SSHORT:
        result = (T) * (ensure_pdata<short>(column));
        return;
    case SQL_C_USHORT:
        result = (T) * (ensure_pdata<unsigned short>(column));
        return;
    case SQL_C_LONG:
    case SQL_C_SLONG:
        result = (T) * (ensure_pdata<int32_t>(column));
        return;
    case SQL_C_ULONG:
        result = (T) * (ensure_pdata<uint32_t>(column));
        return;
    case SQL_C_FLOAT:
        result = (T) * (ensure_pdata<float>(column));
        return;
    case SQL_C_DOUBLE:
        result = (T) * (ensure_pdata<double>(column));
        return;
    case SQL_C_SBIGINT:
        result = (T) * (ensure_pdata<int64_t>(column));
        return;
    case SQL_C_UBIGINT:
        result = (T) * (ensure_pdata<uint64_t>(column));
        return;
    }
    throw type_incompatible_error();
}

} // namespace nanodbc

// clang-format off
// 8888888888                            8888888888                         888    d8b
// 888                                   888                                888    Y8P
// 888                                   888                                888
// 8888888 888d888 .d88b.   .d88b.       8888888 888  888 88888b.   .d8888b 888888 888  .d88b.  88888b.  .d8888b
// 888     888P"  d8P  Y8b d8P  Y8b      888     888  888 888 "88b d88P"    888    888 d88""88b 888 "88b 88K
// 888     888    88888888 88888888      888     888  888 888  888 888      888    888 888  888 888  888 "Y8888b.
// 888     888    Y8b.     Y8b.          888     Y88b 888 888  888 Y88b.    Y88b.  888 Y88..88P 888  888      X88
// 888     888     "Y8888   "Y8888       888      "Y88888 888  888  "Y8888P  "Y888 888  "Y88P"  888  888  88888P'
// MARK: Free Functions -
// clang-format on

namespace nanodbc
{

std::list<datasource> list_datasources()
{
    NANODBC_SQLCHAR name[1024] = {0};
    NANODBC_SQLCHAR driver[1024] = {0};
    SQLSMALLINT name_len_ret{0};
    SQLSMALLINT driver_len_ret{0};
    SQLUSMALLINT direction{SQL_FETCH_FIRST};

    connection env; // ensures handles RAII
    env.allocate();
    NANODBC_ASSERT(env.native_env_handle());

    std::list<datasource> dsns;
    RETCODE rc{SQL_SUCCESS};
    do
    {
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLDataSources),
            rc,
            env.native_env_handle(),                  // EnvironmentHandle
            direction,                                // Direction
            name,                                     // DSN Name
            sizeof(name) / sizeof(NANODBC_SQLCHAR),   // Size of Name Buffer
            &name_len_ret,                            // Written DSN Name length
            driver,                                   // Driver Name
            sizeof(driver) / sizeof(NANODBC_SQLCHAR), // Size of Driver Buffer
            &driver_len_ret);                         // Written Driver length

        if (rc == SQL_SUCCESS)
        {
            using char_type = string::value_type;
            static_assert(
                sizeof(NANODBC_SQLCHAR) == sizeof(char_type),
                "incompatible SQLCHAR and string::value_type");

            datasource dsn;
            dsn.name = string(&name[0], &name[std::char_traits<NANODBC_SQLCHAR>::length(name)]);
            dsn.driver =
                string(&driver[0], &driver[std::char_traits<NANODBC_SQLCHAR>::length(driver)]);

            dsns.push_back(std::move(dsn));
            direction = SQL_FETCH_NEXT;
        }
        else
        {
            if (rc != SQL_NO_DATA)
                NANODBC_THROW_DATABASE_ERROR(env.native_env_handle(), SQL_HANDLE_ENV);
        }
    } while (success(rc));

    return dsns;
}

std::list<driver> list_drivers()
{
    NANODBC_SQLCHAR descr[1024] = {0};
    NANODBC_SQLCHAR attrs[1024] = {0};
    SQLSMALLINT descr_len_ret{0};
    SQLSMALLINT attrs_len_ret{0};
    SQLUSMALLINT direction{SQL_FETCH_FIRST};

    connection env; // ensures handles RAII
    env.allocate();
    NANODBC_ASSERT(env.native_env_handle());

    std::list<driver> drivers;
    RETCODE rc{SQL_SUCCESS};
    do
    {
        NANODBC_CALL_RC(
            NANODBC_FUNC(SQLDrivers),
            rc,
            env.native_env_handle(),
            direction,                               // EnvironmentHandle
            descr,                                   // DriverDescription
            sizeof(descr) / sizeof(NANODBC_SQLCHAR), // BufferLength1
            &descr_len_ret,                          // DescriptionLengthPtr
            attrs,                                   // DriverAttributes
            sizeof(attrs) / sizeof(NANODBC_SQLCHAR), // BufferLength2
            &attrs_len_ret);                         // AttributesLengthPtr

        if (rc == SQL_SUCCESS)
        {
            using char_type = string::value_type;
            static_assert(
                sizeof(NANODBC_SQLCHAR) == sizeof(char_type),
                "incompatible SQLCHAR and string::value_type");

            driver drv;
            drv.name = string(&descr[0], &descr[std::char_traits<NANODBC_SQLCHAR>::length(descr)]);

            // Split "Key1=Value1\0Key2=Value2\0\0" into list of key-value pairs
            auto beg = &attrs[0];
            auto const end = &attrs[attrs_len_ret];
            for (auto pair_end = std::find(beg, end, NANODBC_TEXT('\0')); pair_end != end;
                 pair_end = std::find(beg, end, NANODBC_TEXT('\0')))
            {
                auto const eq_pos = std::find(beg, pair_end, NANODBC_TEXT('='));
                if (eq_pos == end)
                    break;

                driver::attribute attr{{beg, eq_pos}, {eq_pos + 1, pair_end}};
                drv.attributes.push_back(std::move(attr));
                beg = pair_end + 1;
            }

            drivers.push_back(std::move(drv));

            direction = SQL_FETCH_NEXT;
        }
        else
        {
            if (rc != SQL_NO_DATA)
                NANODBC_THROW_DATABASE_ERROR(env.native_env_handle(), SQL_HANDLE_ENV);
        }
    } while (success(rc));

    return drivers;
}

result execute(connection& conn, string const& query, long batch_operations, long timeout)
{
    class statement statement;
    batch_ops array_sizes;
    array_sizes.rowset_size = batch_operations;
    return statement.execute_direct(conn, query, array_sizes, timeout);
}

void just_execute(connection& conn, string const& query, long batch_operations, long timeout)
{
    class statement statement;
    statement.just_execute_direct(conn, query, batch_operations, timeout);
}

result execute(statement& stmt, long batch_operations)
{
    return stmt.execute(batch_operations);
}

void just_execute(statement& stmt, long batch_operations)
{
    return stmt.just_execute(batch_operations);
}

result transact(statement& stmt, long batch_operations)
{
    class transaction transaction(stmt.connection());
    result rvalue = stmt.execute(batch_operations);
    transaction.commit();
    return rvalue;
}

void just_transact(statement& stmt, long batch_operations)
{
    class transaction transaction(stmt.connection());
    stmt.just_execute(batch_operations);
    transaction.commit();
}

void prepare(statement& stmt, string const& query, long timeout)
{
    stmt.prepare(stmt.connection(), query, timeout);
}

} // namespace nanodbc

// clang-format off
//  .d8888b.                                               888    d8b                             8888888888                 888
// d88P  Y88b                                              888    Y8P                             888                        888
// 888    888                                              888                                    888                        888
// 888         .d88b.  88888b.  88888b.   .d88b.   .d8888b 888888 888  .d88b.  88888b.            8888888 888  888  888  .d88888
// 888        d88""88b 888 "88b 888 "88b d8P  Y8b d88P"    888    888 d88""88b 888 "88b           888     888  888  888 d88" 888
// 888    888 888  888 888  888 888  888 88888888 888      888    888 888  888 888  888           888     888  888  888 888  888
// Y88b  d88P Y88..88P 888  888 888  888 Y8b.     Y88b.    Y88b.  888 Y88..88P 888  888           888     Y88b 888 d88P Y88b 888
//  "Y8888P"   "Y88P"  888  888 888  888  "Y8888   "Y8888P  "Y888 888  "Y88P"  888  888           888      "Y8888888P"   "Y88888
// MARK: Connection Fwd -
// clang-format on

namespace nanodbc
{

connection::connection()
    : impl_(new connection_impl())
{
}

connection::connection(const connection& rhs)
    : impl_(rhs.impl_)
{
}

connection::connection(connection&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
{
}

connection& connection::operator=(connection rhs)
{
    swap(rhs);
    return *this;
}

void connection::swap(connection& rhs) noexcept
{
    using std::swap;
    swap(impl_, rhs.impl_);
}

connection::connection(string const& dsn, string const& user, string const& pass, long timeout)
    : impl_(new connection_impl(dsn, user, pass, timeout))
{
}

connection::connection(string const& connection_string, long timeout)
    : impl_(new connection_impl(connection_string, timeout))
{
}

#ifdef NANODBC_HAS_STD_VARIANT
connection::connection(
    string const& dsn,
    string const& user,
    string const& pass,
    std::list<attribute> const& attributes)
    : impl_(new connection_impl(dsn, user, pass, attributes))
{
}

connection::connection(string const& connection_string, std::list<attribute> const& attributes)
    : impl_(new connection_impl(connection_string, attributes))
{
}
#endif

connection::~connection() noexcept {}

void connection::allocate()
{
    impl_->allocate();
}

void connection::deallocate()
{
    impl_->deallocate();
}

void connection::connect(string const& dsn, string const& user, string const& pass, long timeout)
{
    impl_->connect(dsn, user, pass, timeout);
}

void connection::connect(string const& connection_string, long timeout)
{
    impl_->connect(connection_string, timeout);
}

#ifdef NANODBC_HAS_STD_VARIANT
void connection::connect(
    string const& dsn,
    string const& user,
    string const& pass,
    std::list<attribute> const& attributes)
{
    impl_->connect(dsn, user, pass, attributes);
}

void connection::connect(string const& connection_string, std::list<attribute> const& attributes)
{
    impl_->connect(connection_string, attributes);
}
#endif

#if !defined(NANODBC_DISABLE_ASYNC) && defined(SQL_ATTR_ASYNC_DBC_EVENT)
bool connection::async_connect(
    string const& dsn,
    string const& user,
    string const& pass,
    void* event_handle,
    long timeout)
{
    return impl_->connect(dsn, user, pass, timeout, event_handle) == SQL_STILL_EXECUTING;
}

bool connection::async_connect(string const& connection_string, void* event_handle, long timeout)
{
    return impl_->connect(connection_string, timeout, event_handle) == SQL_STILL_EXECUTING;
}

void connection::async_complete()
{
    impl_->async_complete();
}
#endif // !NANODBC_DISABLE_ASYNC && SQL_ATTR_ASYNC_DBC_EVENT

bool connection::connected() const
{
    return impl_->connected();
}

void connection::disconnect()
{
    impl_->disconnect();
}

std::size_t connection::transactions() const
{
    return impl_->transactions();
}

template <class T>
T connection::get_info(short info_type) const
{
    return impl_->get_info<T>(info_type);
}

void* connection::native_dbc_handle() const
{
    return impl_->native_dbc_handle();
}

void* connection::native_env_handle() const
{
    return impl_->native_env_handle();
}

string connection::dbms_name() const
{
    return impl_->dbms_name();
}

string connection::dbms_version() const
{
    return impl_->dbms_version();
}

string connection::driver_name() const
{
    return impl_->driver_name();
}

string connection::driver_version() const
{
    return impl_->driver_version();
}

string connection::database_name() const
{
    return impl_->database_name();
}

string connection::catalog_name() const
{
    return impl_->catalog_name();
}

std::size_t connection::ref_transaction()
{
    return impl_->ref_transaction();
}

std::size_t connection::unref_transaction()
{
    return impl_->unref_transaction();
}

bool connection::rollback() const
{
    return impl_->rollback();
}

void connection::rollback(bool onoff)
{
    impl_->rollback(onoff);
}

} // namespace nanodbc

// clang-format off
// 88888888888                                                  888    d8b                             8888888888                 888
//     888                                                      888    Y8P                             888                        888
//     888                                                      888                                    888                        888
//     888  888d888 8888b.  88888b.  .d8888b   8888b.   .d8888b 888888 888  .d88b.  88888b.            8888888 888  888  888  .d88888 .d8888b
//     888  888P"      "88b 888 "88b 88K          "88b d88P"    888    888 d88""88b 888 "88b           888     888  888  888 d88" 888 88K
//     888  888    .d888888 888  888 "Y8888b. .d888888 888      888    888 888  888 888  888           888     888  888  888 888  888 "Y8888b.
//     888  888    888  888 888  888      X88 888  888 Y88b.    Y88b.  888 Y88..88P 888  888           888     Y88b 888 d88P Y88b 888      X88
//     888  888    "Y888888 888  888  88888P' "Y888888  "Y8888P  "Y888 888  "Y88P"  888  888           888      "Y8888888P"   "Y88888  88888P'
// MARK: Transaction Fwd -
// clang-format on

namespace nanodbc
{

transaction::transaction(const class connection& conn)
    : impl_(new transaction_impl(conn))
{
}

transaction::transaction(const transaction& rhs)
    : impl_(rhs.impl_)
{
}

transaction::transaction(transaction&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
{
}

transaction& transaction::operator=(transaction rhs)
{
    swap(rhs);
    return *this;
}

void transaction::swap(transaction& rhs) noexcept
{
    using std::swap;
    swap(impl_, rhs.impl_);
}

transaction::~transaction() noexcept {}

void transaction::commit()
{
    impl_->commit();
}

void transaction::rollback() noexcept
{
    impl_->rollback();
}

class connection& transaction::connection()
{
    return impl_->connection();
}

const class connection& transaction::connection() const
{
    return impl_->connection();
}

transaction::operator class connection &()
{
    return impl_->connection();
}

transaction::operator const class connection &() const
{
    return impl_->connection();
}

} // namespace nanodbc

// clang-format off
//  .d8888b.  888             888                                            888              8888888888                 888
// d88P  Y88b 888             888                                            888              888                        888
// Y88b.      888             888                                            888              888                        888
//  "Y888b.   888888  8888b.  888888 .d88b.  88888b.d88b.   .d88b.  88888b.  888888           8888888 888  888  888  .d88888
//     "Y88b. 888        "88b 888   d8P  Y8b 888 "888 "88b d8P  Y8b 888 "88b 888              888     888  888  888 d88" 888
//       "888 888    .d888888 888   88888888 888  888  888 88888888 888  888 888              888     888  888  888 888  888
// Y88b  d88P Y88b.  888  888 Y88b. Y8b.     888  888  888 Y8b.     888  888 Y88b.            888     Y88b 888 d88P Y88b 888
//  "Y8888P"   "Y888 "Y888888  "Y888 "Y8888  888  888  888  "Y8888  888  888  "Y888           888      "Y8888888P"   "Y88888
// MARK: Statement Fwd -
// clang-format on

namespace nanodbc
{

statement::statement()
    : impl_(new statement_impl())
{
}

statement::statement(class connection& conn)
    : impl_(new statement_impl(conn))
{
}

statement::statement(class connection& conn, std::list<attribute> const& attributes)
    : impl_(new statement_impl(conn, attributes))
{
}

statement::statement(statement&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
{
}

statement::statement(class connection& conn, string const& query, long timeout)
    : impl_(new statement_impl(conn, query, timeout))
{
}

statement::statement(const statement& rhs)
    : impl_(rhs.impl_)
{
}

statement& statement::operator=(statement rhs)
{
    swap(rhs);
    return *this;
}

void statement::swap(statement& rhs) noexcept
{
    using std::swap;
    swap(impl_, rhs.impl_);
}

statement::~statement() noexcept {}

void statement::open(class connection& conn)
{
    impl_->open(conn);
}

bool statement::open() const
{
    return impl_->open();
}

bool statement::connected() const
{
    return impl_->connected();
}

const class connection& statement::connection() const
{
    return impl_->connection();
}

class connection& statement::connection()
{
    return impl_->connection();
}

void* statement::native_statement_handle() const
{
    return impl_->native_statement_handle();
}

void statement::close()
{
    impl_->close();
}

void statement::cancel()
{
    impl_->cancel();
}

void statement::prepare(class connection& conn, string const& query, long timeout)
{
    impl_->prepare(conn, query, timeout);
}

void statement::prepare(string const& query, long timeout)
{
    impl_->prepare(query, timeout);
}

void statement::timeout(long timeout)
{
    impl_->timeout(timeout);
}

result statement::execute_direct(
    class connection& conn,
    string const& query,
    long batch_operations,
    long timeout)
{
    return impl_->execute_direct(conn, query, batch_operations, timeout, *this);
}

result statement::execute_direct(
    class connection& conn,
    string const& query,
    batch_ops const& array_sizes,
    long timeout)
{
    return impl_->execute_direct(conn, query, array_sizes, timeout, *this);
}

#if defined(NANODBC_DO_ASYNC_IMPL)
bool statement::async_prepare(string const& query, void* event_handle, long timeout)
{
    return impl_->async_prepare(query, event_handle, timeout);
}

bool statement::async_execute_direct(
    class connection& conn,
    void* event_handle,
    string const& query,
    long batch_operations,
    long timeout)
{
    return impl_->async_execute_direct(conn, event_handle, query, batch_operations, timeout, *this);
}

bool statement::async_execute(void* event_handle, long batch_operations, long timeout)
{
    return impl_->async_execute(event_handle, batch_operations, timeout, *this);
}

void statement::complete_prepare()
{
    return impl_->complete_prepare();
}

result statement::complete_execute(long batch_operations)
{
    return impl_->complete_execute(batch_operations, *this);
}

void statement::enable_async(void* event_handle)
{
    impl_->enable_async(event_handle);
}

void statement::disable_async() const
{
    impl_->disable_async();
}
#endif

void statement::just_execute_direct(
    class connection& conn,
    string const& query,
    long batch_operations,
    long timeout)
{
    impl_->just_execute_direct(conn, query, batch_operations, timeout, *this);
}

result statement::execute(long batch_operations, long timeout)
{
    return impl_->execute(batch_operations, timeout, *this);
}

void statement::just_execute(long batch_operations, long timeout)
{
    impl_->just_execute(batch_operations, timeout, *this);
}

result statement::procedure_columns(
    string const& catalog,
    string const& schema,
    string const& procedure,
    string const& column)
{
    return impl_->procedure_columns(catalog, schema, procedure, column, *this);
}

long statement::affected_rows() const
{
    return impl_->affected_rows();
}

short statement::columns() const
{
    return impl_->columns();
}

short statement::parameters() const
{
    return impl_->parameters();
}

void statement::reset_parameters() noexcept
{
    impl_->reset_parameters();
}

unsigned long statement::parameter_size(short param_index) const
{
    return impl_->parameter_size(param_index);
}

short statement::parameter_scale(short param_index) const
{
    return impl_->parameter_scale(param_index);
}

short statement::parameter_type(short param_index) const
{
    return impl_->parameter_type(param_index);
}

// We need to instantiate each form of bind() for each of our supported data types.
#define NANODBC_INSTANTIATE_BINDS(type)                                                            \
    template void statement::bind(short, const type*, param_direction);              /* 1-ary */   \
    template void statement::bind(short, const type*, std::size_t, param_direction); /* n-ary */   \
    template void statement::bind(                                                                 \
        short, const type*, std::size_t, const type*, param_direction); /* n-ary, sentry */        \
    template void statement::bind(                                                                 \
        short, const type*, std::size_t, const bool*, param_direction) /* n-ary, flags */

#define NANODBC_INSTANTIATE_BIND_VECTOR_STRINGS(type)                                              \
    template void statement::bind_strings(short, std::vector<type> const&, param_direction);       \
    template void statement::bind_strings(                                                         \
        short, std::vector<type> const&, type::value_type const*, param_direction);                \
    template void statement::bind_strings(                                                         \
        short, std::vector<type> const&, bool const*, param_direction);

#define NANODBC_INSTANTIATE_BIND_STRINGS(type)                                                     \
    NANODBC_INSTANTIATE_BIND_VECTOR_STRINGS(type)                                                  \
    template void statement::bind_strings(                                                         \
        short, const type::value_type*, std::size_t, std::size_t, param_direction);                \
    template void statement::bind_strings(                                                         \
        short,                                                                                     \
        type::value_type const*,                                                                   \
        std::size_t,                                                                               \
        std::size_t,                                                                               \
        type::value_type const*,                                                                   \
        param_direction);                                                                          \
    template void statement::bind_strings(                                                         \
        short, type::value_type const*, std::size_t, std::size_t, bool const*, param_direction)

// The following are the only supported instantiations of statement::bind().
NANODBC_INSTANTIATE_BINDS(std::string::value_type);
NANODBC_INSTANTIATE_BINDS(wide_string::value_type);
NANODBC_INSTANTIATE_BINDS(short);
NANODBC_INSTANTIATE_BINDS(unsigned short);
NANODBC_INSTANTIATE_BINDS(int);
NANODBC_INSTANTIATE_BINDS(unsigned int);
NANODBC_INSTANTIATE_BINDS(long int);
NANODBC_INSTANTIATE_BINDS(unsigned long int);
NANODBC_INSTANTIATE_BINDS(long long);
NANODBC_INSTANTIATE_BINDS(unsigned long long);
NANODBC_INSTANTIATE_BINDS(float);
NANODBC_INSTANTIATE_BINDS(double);
NANODBC_INSTANTIATE_BINDS(date);
NANODBC_INSTANTIATE_BINDS(time);
NANODBC_INSTANTIATE_BINDS(timestamp);

NANODBC_INSTANTIATE_BIND_STRINGS(std::string);
NANODBC_INSTANTIATE_BIND_STRINGS(wide_string);

#ifdef NANODBC_HAS_STD_STRING_VIEW
NANODBC_INSTANTIATE_BIND_VECTOR_STRINGS(std::string_view);
NANODBC_INSTANTIATE_BIND_VECTOR_STRINGS(wide_string_view);
#endif

#undef NANODBC_INSTANTIATE_BINDS
#undef NANODBC_INSTANTIATE_BIND_STRINGS

template <class T>
void statement::bind(short param_index, const T* value, param_direction direction)
{
    impl_->bind(direction, param_index, value, 1);
}

template <class T>
void statement::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    param_direction direction)
{
    impl_->bind(direction, param_index, values, batch_size);
}

template <class T>
void statement::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    T const* null_sentry,
    param_direction direction)
{
    impl_->bind(direction, param_index, values, batch_size, nullptr, null_sentry);
}

template <class T>
void statement::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    bool const* nulls,
    param_direction direction)
{
    impl_->bind(direction, param_index, values, batch_size, nulls);
}

void statement::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values,
    param_direction direction)
{
    impl_->bind(direction, param_index, values);
}

void statement::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values,
    bool const* nulls,
    param_direction direction)
{
    impl_->bind(direction, param_index, values, nulls);
}

void statement::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values,
    uint8_t const* null_sentry,
    param_direction direction)
{
    impl_->bind(direction, param_index, values, nullptr, null_sentry);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    std::vector<T> const& values,
    param_direction direction)
{
    impl_->bind_strings(direction, param_index, values);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    param_direction direction)
{
    impl_->bind_strings(direction, param_index, values, value_size, batch_size);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    T const* null_sentry,
    param_direction direction)
{
    impl_->bind_strings(
        direction, param_index, values, value_size, batch_size, nullptr, null_sentry);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    bool const* nulls,
    param_direction direction)
{
    impl_->bind_strings(direction, param_index, values, value_size, batch_size, nulls);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    std::vector<T> const& values,
    typename T::value_type const* null_sentry,
    param_direction direction)
{
    impl_->bind_strings(direction, param_index, values, nullptr, null_sentry);
}

template <class T, typename>
void statement::bind_strings(
    short param_index,
    std::vector<T> const& values,
    bool const* nulls,
    param_direction direction)
{
    impl_->bind_strings(direction, param_index, values, nulls);
}

void statement::bind_null(short param_index, std::size_t batch_size)
{
    impl_->bind_null(param_index, batch_size);
}

void statement::describe_parameters(
    const std::vector<short>& idx,
    const std::vector<short>& type,
    const std::vector<unsigned long>& size,
    const std::vector<short>& scale)
{
    impl_->describe_parameters(idx, type, size, scale);
}

} // namespace nanodbc

// clang-format off
// 8888888b.                                     d8b          888
// 888  "Y88b                                    Y8P          888
// 888    888                                                 888
// 888    888  .d88b.  .d8888b   .d8888b 888d888 888 88888b.  888888 .d88b.  888d888 .d8888b
// 888    888 d8P  Y8b 88K      d88P"    888P"   888 888 "88b 888   d88""88b 888P"   88K
// 888    888 88888888 "Y8888b. 888      888     888 888  888 888   888  888 888     "Y8888b.
// 888  .d88P Y8b.          X88 Y88b.    888     888 888 d88P Y88b. Y88..88P 888          X88
// 8888888P"   "Y8888   88888P'  "Y8888P 888     888 88888P"   "Y888 "Y88P"  888      88888P'
//                                                   888
//                                                   888
//                                                   888
// MARK: Descriptors -
// clang-format on
namespace nanodbc
{

implementation_row_descriptor::sql_get_descr_field::sql_get_descr_field(
    implementation_row_descriptor const& ird,
    short record,
    std::uint16_t field_identifier) noexcept
    : ird_(ird)
    , record_(record)
    , field_identifier_(field_identifier)
{
}

implementation_row_descriptor::sql_get_descr_field::operator std::int64_t() const
{
    // If ValuePtr is integer type, applications should use a buffer of SQLULEN and initialize the
    // value to 0 before calling this function as some drivers may only write the lower 32-bit or
    // 16-bit of a buffer and leave the higher-order bit unchanged.
    SQLULEN value = 0;
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetDescField),
        rc,
        ird_.descriptor_handle_,
        static_cast<SQLUSMALLINT>(record_ + 1),
        field_identifier_,
        &value,
        0,
        nullptr);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(ird_.statement_handle_, SQL_HANDLE_STMT);

    return value;
}

implementation_row_descriptor::sql_get_descr_field::operator std::uint64_t() const
{
    // If ValuePtr is integer type, applications should use a buffer of SQLULEN and initialize the
    // value to 0 before calling this function as some drivers may only write the lower 32-bit or
    // 16-bit of a buffer and leave the higher-order bit unchanged.
    SQLULEN value = 0;
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetDescField),
        rc,
        ird_.descriptor_handle_,
        static_cast<SQLUSMALLINT>(record_ + 1),
        field_identifier_,
        &value,
        0,
        nullptr);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(ird_.statement_handle_, SQL_HANDLE_STMT);

    return value;
}

implementation_row_descriptor::sql_get_descr_field::operator string() const
{
    NANODBC_SQLCHAR value[512] = {0};
    SQLINTEGER len = 0; // total number of bytes
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetDescField),
        rc,
        ird_.descriptor_handle_,
        static_cast<SQLUSMALLINT>(record_ + 1),
        field_identifier_,
        value,
        sizeof(value) / sizeof(NANODBC_SQLCHAR),
        &len);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(ird_.statement_handle_, SQL_HANDLE_STMT);

    NANODBC_ASSERT(len % sizeof(NANODBC_SQLCHAR) == 0);
    len = len / sizeof(NANODBC_SQLCHAR);

    NANODBC_ASSERT(len <= std::char_traits<NANODBC_SQLCHAR>::length(value));
    return {&value[0], &value[len]};
}

implementation_row_descriptor::implementation_row_descriptor(result const& result)
    : statement_handle_(result.native_statement_handle())
    , statement_columns_count_(result.columns())
{
    initialize_descriptor();
}

implementation_row_descriptor::implementation_row_descriptor(statement const& statement)
    : statement_handle_(statement.native_statement_handle())
    , statement_columns_count_(statement.columns())
{
    initialize_descriptor();
}

void implementation_row_descriptor::initialize_descriptor()
{
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetStmtAttr),
        rc,
        statement_handle_,
        SQL_ATTR_IMP_ROW_DESC,
        &descriptor_handle_,
        0,
        nullptr);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(statement_handle_, SQL_HANDLE_STMT);

    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLGetDescField),
        rc,
        descriptor_handle_,
        0,
        SQL_DESC_COUNT,
        &descriptor_records_count_,
        0,
        nullptr);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(statement_handle_, SQL_HANDLE_STMT);
}

void implementation_row_descriptor::throw_if_record_is_out_of_range(short record) const
{
    if ((record < 0) || (record >= descriptor_records_count_))
        throw index_range_error();
}

// Descriptor header fields accessors

auto implementation_row_descriptor::alloc_type() const -> short
{
    std::int64_t const value = sql_get_descr_field(*this, 0, SQL_DESC_ALLOC_TYPE);
    return static_cast<short>(value);
}

short implementation_row_descriptor::count() const noexcept
{
    return descriptor_records_count_;
}

// Descriptor record fields (records) accessors

auto implementation_row_descriptor::auto_unique_value(short record) const -> bool
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_AUTO_UNIQUE_VALUE);
    return value == SQL_TRUE;
}

auto implementation_row_descriptor::base_column_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_BASE_COLUMN_NAME);
}

auto implementation_row_descriptor::base_table_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_BASE_TABLE_NAME);
}

auto implementation_row_descriptor::case_sensitive(short record) const -> bool
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_CASE_SENSITIVE);
    if (value == SQL_TRUE)
        return true;
    else if (value == SQL_FALSE)
        return false;
    else
        throw std::out_of_range("SQL_DESC_UNNAMED value unknown");
}

auto implementation_row_descriptor::catalog_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_CATALOG_NAME);
}

auto implementation_row_descriptor::concise_type(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_CONCISE_TYPE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::display_size(short record) const -> std::int64_t
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_DISPLAY_SIZE);
}

auto implementation_row_descriptor::fixed_prec_scale(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_FIXED_PREC_SCALE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::label(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_LABEL);
}

auto implementation_row_descriptor::length(short record) const -> std::uint64_t
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_LENGTH);
}

auto implementation_row_descriptor::local_type_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_LOCAL_TYPE_NAME);
}

auto implementation_row_descriptor::name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_NAME);
}

auto implementation_row_descriptor::nullable(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_NULLABLE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::num_prec_radix(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_NUM_PREC_RADIX);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::octet_length(short record) const -> std::int64_t
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_OCTET_LENGTH);
}

auto implementation_row_descriptor::precision(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_PRECISION);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::rowver(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_ROWVER);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::scale(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_SCALE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::schema_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_SCHEMA_NAME);
}

auto implementation_row_descriptor::searchable(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_SEARCHABLE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::table_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_TABLE_NAME);
}

auto implementation_row_descriptor::type(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_TYPE);
    return static_cast<short>(value);
}

auto implementation_row_descriptor::type_name(short record) const -> string
{
    throw_if_record_is_out_of_range(record);
    return sql_get_descr_field(*this, record, SQL_DESC_TYPE_NAME);
}

auto implementation_row_descriptor::unnamed(short record) const -> bool
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_UNNAMED);
    if (value == SQL_NAMED)
        return false;
    else if (value == SQL_UNNAMED)
        return true;
    else
        throw std::out_of_range("SQL_DESC_UNNAMED value unknown");
}

auto implementation_row_descriptor::unsigned_(short record) const -> bool
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_UNSIGNED);
    if (value == SQL_TRUE)
        return true;
    else if (value == SQL_FALSE)
        return false;
    else
        throw std::out_of_range("SQL_DESC_UNNAMED value unknown");
}

auto implementation_row_descriptor::updatable(short record) const -> short
{
    throw_if_record_is_out_of_range(record);
    std::int64_t const value = sql_get_descr_field(*this, record, SQL_DESC_UPDATABLE);
    return static_cast<short>(value);
}

} // namespace nanodbc

// clang-format off
// 888b     d888  .d8888b.   .d8888b.   .d88888b.  888                       88888888888 888     888 8888888b.
// 8888b   d8888 d88P  Y88b d88P  Y88b d88P" "Y88b 888                           888     888     888 888   Y88b
// 88888b.d88888 Y88b.      Y88b.      888     888 888                           888     888     888 888    888
// 888Y88888P888  "Y888b.    "Y888b.   888     888 888                           888     Y88b   d88P 888   d88P
// 888 Y888P 888     "Y88b.     "Y88b. 888     888 888                           888      Y88b d88P  8888888P"
// 888  Y8P  888       "888       "888 888 Y8b 888 888           888888          888       Y88o88P   888
// 888   "   888 Y88b  d88P Y88b  d88P Y88b.Y8b88P 888                           888        Y888P    888
// 888       888  "Y8888P"   "Y8888P"   "Y888888"  88888888                      888         Y8P     888
//                                            Y8b
// MARK: MSSQL - TVP (Table Valued Parameters) -
// clang-format on
#ifndef NANODBC_DISABLE_MSSQL_TVP
namespace nanodbc
{

table_valued_parameter::table_valued_parameter()
    : impl_(new table_valued_parameter_impl())
{
}

table_valued_parameter::table_valued_parameter(const table_valued_parameter& rhs)
    : impl_(rhs.impl_)
{
}

table_valued_parameter::table_valued_parameter(table_valued_parameter&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
{
}

table_valued_parameter::table_valued_parameter(statement& stmt, short param_index, size_t row_count)
    : impl_(new table_valued_parameter_impl())
{
    impl_->open(*this, stmt, param_index, row_count);
}

table_valued_parameter::~table_valued_parameter() noexcept {}

void table_valued_parameter::open(statement& stmt, short param_index, size_t row_count)
{
    impl_->open(*this, stmt, param_index, row_count);
}

void table_valued_parameter::close()
{
    impl_->close();
}

// We need to instantiate each form of bind() for each of our supported data types.
#define NANODBC_INSTANTIATE_TVP_BINDS(type)                                                        \
    template void table_valued_parameter::bind(short, const type*, std::size_t); /* n-ary */       \
    template void table_valued_parameter::bind(                                                    \
        short, const type*, std::size_t, const type*); /* n-ary, sentry */                         \
    template void table_valued_parameter::bind(                                                    \
        short, const type*, std::size_t, const bool*) /* n-ary, flags */

#define NANODBC_INSTANTIATE_TVP_BIND_VECTOR_STRINGS(type)                                          \
    template void table_valued_parameter::bind_strings(short, std::vector<type> const&);           \
    template void table_valued_parameter::bind_strings(                                            \
        short, std::vector<type> const&, type::value_type const*);                                 \
    template void table_valued_parameter::bind_strings(                                            \
        short, std::vector<type> const&, bool const*);

#define NANODBC_INSTANTIATE_TVP_BIND_STRINGS(type)                                                 \
    NANODBC_INSTANTIATE_TVP_BIND_VECTOR_STRINGS(type)                                              \
    template void table_valued_parameter::bind_strings(                                            \
        short, const type::value_type*, std::size_t, std::size_t);                                 \
    template void table_valued_parameter::bind_strings(                                            \
        short, type::value_type const*, std::size_t, std::size_t, type::value_type const*);        \
    template void table_valued_parameter::bind_strings(                                            \
        short, type::value_type const*, std::size_t, std::size_t, bool const*)
// The following are the only supported instantiations of statement::bind().
NANODBC_INSTANTIATE_TVP_BINDS(std::string::value_type);
NANODBC_INSTANTIATE_TVP_BINDS(wide_string::value_type);
NANODBC_INSTANTIATE_TVP_BINDS(short);
NANODBC_INSTANTIATE_TVP_BINDS(unsigned short);
NANODBC_INSTANTIATE_TVP_BINDS(int);
NANODBC_INSTANTIATE_TVP_BINDS(unsigned int);
NANODBC_INSTANTIATE_TVP_BINDS(long int);
NANODBC_INSTANTIATE_TVP_BINDS(unsigned long int);
NANODBC_INSTANTIATE_TVP_BINDS(long long);
NANODBC_INSTANTIATE_TVP_BINDS(unsigned long long);
NANODBC_INSTANTIATE_TVP_BINDS(float);
NANODBC_INSTANTIATE_TVP_BINDS(double);
NANODBC_INSTANTIATE_TVP_BINDS(date);
NANODBC_INSTANTIATE_TVP_BINDS(time);
NANODBC_INSTANTIATE_TVP_BINDS(timestamp);

NANODBC_INSTANTIATE_TVP_BIND_STRINGS(std::string);
NANODBC_INSTANTIATE_TVP_BIND_STRINGS(wide_string);

#ifdef NANODBC_HAS_STD_STRING_VIEW
NANODBC_INSTANTIATE_TVP_BIND_VECTOR_STRINGS(std::string_view);
NANODBC_INSTANTIATE_TVP_BIND_VECTOR_STRINGS(wide_string_view);
#endif

#undef NANODBC_INSTANTIATE_TVP_BINDS
#undef NANODBC_INSTANTIATE_TVP_BIND_STRINGS

template <class T>
void table_valued_parameter::bind(short param_index, T const* values, std::size_t batch_size)
{
    impl_->bind(param_index, values, batch_size);
}

template <class T>
void table_valued_parameter::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    T const* null_sentry)
{
    impl_->bind(param_index, values, batch_size, nullptr, null_sentry);
}

template <class T>
void table_valued_parameter::bind(
    short param_index,
    T const* values,
    std::size_t batch_size,
    bool const* nulls)
{
    impl_->bind(param_index, values, batch_size, nulls);
}

void table_valued_parameter::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values)
{
    impl_->bind(param_index, values);
}

void table_valued_parameter::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values,
    bool const* nulls)
{
    impl_->bind(param_index, values, nulls);
}

void table_valued_parameter::bind(
    short param_index,
    std::vector<std::vector<uint8_t>> const& values,
    uint8_t const* null_sentry)
{
    impl_->bind(param_index, values, nullptr, null_sentry);
}

template <class T, typename>
void table_valued_parameter::bind_strings(short param_index, std::vector<T> const& values)
{
    impl_->bind_strings(param_index, values);
}

template <class T, typename>
void table_valued_parameter::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size)
{
    impl_->bind_strings(param_index, values, value_size, batch_size);
}

template <class T, typename>
void table_valued_parameter::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    T const* null_sentry)
{
    impl_->bind_strings(param_index, values, value_size, batch_size, nullptr, null_sentry);
}

template <class T, typename>
void table_valued_parameter::bind_strings(
    short param_index,
    T const* values,
    std::size_t value_size,
    std::size_t batch_size,
    bool const* nulls)
{
    impl_->bind_strings(param_index, values, value_size, batch_size, nulls);
}

template <class T, typename>
void table_valued_parameter::bind_strings(
    short param_index,
    std::vector<T> const& values,
    typename T::value_type const* null_sentry)
{
    impl_->bind_strings(param_index, values, nullptr, null_sentry);
}

template <class T, typename>
void table_valued_parameter::bind_strings(
    short param_index,
    std::vector<T> const& values,
    bool const* nulls)
{
    impl_->bind_strings(param_index, values, nulls);
}

void table_valued_parameter::bind_null(short param_index)
{
    impl_->bind_null(param_index);
}

void table_valued_parameter::describe_parameters(
    const std::vector<short>& idx,
    const std::vector<short>& type,
    const std::vector<unsigned long>& size,
    const std::vector<short>& scale)
{
    impl_->describe_parameters(idx, type, size, scale);
}

short table_valued_parameter::parameters() const
{
    return impl_->parameters();
}

unsigned long table_valued_parameter::parameter_size(short param_index) const
{
    return impl_->parameter_size(param_index);
}

short table_valued_parameter::parameter_scale(short param_index) const
{
    return impl_->parameter_scale(param_index);
}

short table_valued_parameter::parameter_type(short param_index) const
{
    return impl_->parameter_type(param_index);
}
} // namespace nanodbc
#endif // NANODBC_DISABLE_MSSQL_TVP

// clang-format off
//  .d8888b.           888             888
// d88P  Y88b          888             888
// 888    888          888             888
// 888         8888b.  888888  8888b.  888  .d88b.   .d88b.
// 888            "88b 888        "88b 888 d88""88b d88P"88b
// 888    888 .d888888 888    .d888888 888 888  888 888  888
// Y88b  d88P 888  888 Y88b.  888  888 888 Y88..88P Y88b 888
//  "Y8888P"  "Y888888  "Y888 "Y888888 888  "Y88P"   "Y88888
//                                                       888
//                                                  Y8b d88P
//                                                   "Y88P"
// MARK: Catalog -
// clang-format on
namespace nanodbc
{

catalog::tables::tables(result& find_result)
    : result_(find_result)
{
}

bool catalog::tables::next()
{
    return result_.next();
}

string catalog::tables::table_catalog() const
{
    // TABLE_CAT might be NULL
    return result_.get<string>(0, string());
}

string catalog::tables::table_schema() const
{
    // TABLE_SCHEM might be NULL
    return result_.get<string>(1, string());
}

string catalog::tables::table_name() const
{
    // TABLE_NAME column is never NULL
    return result_.get<string>(2);
}

string catalog::tables::table_type() const
{
    // TABLE_TYPE column is never NULL
    return result_.get<string>(3);
}

string catalog::tables::table_remarks() const
{
    // REMARKS might be NULL
    return result_.get<string>(4, string());
}

catalog::procedures::procedures(result& find_result)
    : result_(find_result)
{
}

bool catalog::procedures::next()
{
    return result_.next();
}

string catalog::procedures::procedure_catalog() const
{
    // PROCEDURE_CAT may be NULL
    return result_.get<string>(0, string());
}

string catalog::procedures::procedure_schema() const
{
    // PROCEDURE_SCHEM may be NULL
    return result_.get<string>(1, string());
}

string catalog::procedures::procedure_name() const
{
    // PROCEDURE_NAME is never NULL
    return result_.get<string>(2);
}

string catalog::procedures::procedure_remarks() const
{
    // Column indicies 3, 4, 5 and "reserved for future use".
    // PROCEDURE_REMARKS column may be NULL
    return result_.get<string>(6, string());
}

short catalog::procedures::procedure_type() const
{
    // PROCEDURE_TYPE may be NULL
    return result_.get<short>(7, SQL_PT_UNKNOWN);
}

catalog::table_privileges::table_privileges(result& find_result)
    : result_(find_result)
{
}

bool catalog::table_privileges::next()
{
    return result_.next();
}

string catalog::table_privileges::table_catalog() const
{
    // TABLE_CAT might be NULL
    return result_.get<string>(0, string());
}

string catalog::table_privileges::table_schema() const
{
    // TABLE_SCHEM might be NULL
    return result_.get<string>(1, string());
}

string catalog::table_privileges::table_name() const
{
    // TABLE_NAME column is never NULL
    return result_.get<string>(2);
}

string catalog::table_privileges::grantor() const
{
    // GRANTOR might be NULL
    return result_.get<string>(3, string());
}

string catalog::table_privileges::grantee() const
{
    // GRANTEE column is never NULL
    return result_.get<string>(4);
}

string catalog::table_privileges::privilege() const
{
    // PRIVILEGE column is never NULL
    return result_.get<string>(5);
}

string catalog::table_privileges::is_grantable() const
{
    // IS_GRANTABLE might be NULL
    return result_.get<string>(6, string());
}

catalog::primary_keys::primary_keys(result& find_result)
    : result_(find_result)
{
}

bool catalog::primary_keys::next()
{
    return result_.next();
}

string catalog::primary_keys::table_catalog() const
{
    // TABLE_CAT might be NULL
    return result_.get<string>(0, string());
}

string catalog::primary_keys::table_schema() const
{
    // TABLE_SCHEM might be NULL
    return result_.get<string>(1, string());
}

string catalog::primary_keys::table_name() const
{
    // TABLE_NAME is never NULL
    return result_.get<string>(2);
}

string catalog::primary_keys::column_name() const
{
    // COLUMN_NAME is never NULL
    return result_.get<string>(3);
}

short catalog::primary_keys::column_number() const
{
    // KEY_SEQ is never NULL
    return result_.get<short>(4);
}

string catalog::primary_keys::primary_key_name() const
{
    // PK_NAME might be NULL
    return result_.get<string>(5);
}

catalog::procedure_columns::procedure_columns(result& find_result)
    : result_(find_result)
{
}

bool catalog::procedure_columns::next()
{
    return result_.next();
}

string catalog::procedure_columns::procedure_catalog() const
{
    // TABLE_CAT might be NULL
    return result_.get<string>(0, string());
}

string catalog::procedure_columns::procedure_schema() const
{
    // TABLE_SCHEM might be NULL
    return result_.get<string>(1, string());
}

string catalog::procedure_columns::procedure_name() const
{
    // TABLE_NAME is never NULL
    return result_.get<string>(2);
}

string catalog::procedure_columns::column_name() const
{
    // COLUMN_NAME is never NULL
    return result_.get<string>(3);
}

short catalog::procedure_columns::column_type() const
{
    // DATA_TYPE is never NULL
    return result_.get<short>(4);
}

short catalog::procedure_columns::data_type() const
{
    // DATA_TYPE is never NULL
    return result_.get<short>(5);
}

string catalog::procedure_columns::type_name() const
{
    // TYPE_NAME is never NULL
    return result_.get<string>(6);
}

long catalog::procedure_columns::column_size() const
{
    // COLUMN_SIZE, might be NULL
    return result_.get<long>(7, 0);
}

long catalog::procedure_columns::buffer_length() const
{
    // BUFFER_LENGTH
    return result_.get<long>(8);
}

short catalog::procedure_columns::decimal_digits() const
{
    // DECIMAL_DIGITS might be NULL
    return result_.get<short>(9, 0);
}

short catalog::procedure_columns::numeric_precision_radix() const
{
    // NUM_PREC_RADIX might be NULL
    return result_.get<short>(10, 0);
}

short catalog::procedure_columns::nullable() const
{
    // NULLABLE is never NULL
    return result_.get<short>(11);
}

string catalog::procedure_columns::remarks() const
{
    // REMARKS might be NULL
    return result_.get<string>(12, string());
}

string catalog::procedure_columns::column_default() const
{
    // COLUMN_DEF might be NULL, if no default value is specified
    return result_.get<string>(13, string());
}

short catalog::procedure_columns::sql_data_type() const
{
    // SQL_DATA_TYPE is never NULL
    return result_.get<short>(14);
}

short catalog::procedure_columns::sql_datetime_subtype() const
{
    // SQL_DATETIME_SUB might be NULL
    return result_.get<short>(15, 0);
}

long catalog::procedure_columns::char_octet_length() const
{
    // CHAR_OCTET_LENGTH might be NULL
    return result_.get<long>(16, 0);
}

long catalog::procedure_columns::ordinal_position() const
{
    // ORDINAL_POSITION is never NULL
    return result_.get<long>(17);
}

string catalog::procedure_columns::is_nullable() const
{
    // IS_NULLABLE might be NULL.
    return result_.get<string>(18, string());
}

catalog::columns::columns(result& find_result)
    : result_(find_result)
{
}

bool catalog::columns::next()
{
    return result_.next();
}

string catalog::columns::table_catalog() const
{
    // TABLE_CAT might be NULL
    return result_.get<string>(0, string());
}

string catalog::columns::table_schema() const
{
    // TABLE_SCHEM might be NULL
    return result_.get<string>(1, string());
}

string catalog::columns::table_name() const
{
    // TABLE_NAME is never NULL
    return result_.get<string>(2);
}

string catalog::columns::column_name() const
{
    // COLUMN_NAME is never NULL
    return result_.get<string>(3);
}

short catalog::columns::data_type() const
{
    // DATA_TYPE is never NULL
    return result_.get<short>(4);
}

string catalog::columns::type_name() const
{
    // TYPE_NAME is never NULL
    return result_.get<string>(5);
}

long catalog::columns::column_size() const
{
    // COLUMN_SIZE
    return result_.get<long>(6);
}

long catalog::columns::buffer_length() const
{
    // BUFFER_LENGTH
    return result_.get<long>(7);
}

short catalog::columns::decimal_digits() const
{
    // DECIMAL_DIGITS might be NULL
    return result_.get<short>(8, 0);
}

short catalog::columns::numeric_precision_radix() const
{
    // NUM_PREC_RADIX might be NULL
    return result_.get<short>(9, 0);
}

short catalog::columns::nullable() const
{
    // NULLABLE is never NULL
    return result_.get<short>(10);
}

string catalog::columns::remarks() const
{
    // REMARKS might be NULL
    return result_.get<string>(11, string());
}

string catalog::columns::column_default() const
{
    // COLUMN_DEF might be NULL, if no default value is specified
    return result_.get<string>(12, string());
}

short catalog::columns::sql_data_type() const
{
    // SQL_DATA_TYPE is never NULL
    return result_.get<short>(13);
}

short catalog::columns::sql_datetime_subtype() const
{
    // SQL_DATETIME_SUB might be NULL
    return result_.get<short>(14, 0);
}

long catalog::columns::char_octet_length() const
{
    // CHAR_OCTET_LENGTH might be NULL
    return result_.get<long>(15, 0);
}

long catalog::columns::ordinal_position() const
{
    // ORDINAL_POSITION is never NULL
    return result_.get<long>(16);
}

string catalog::columns::is_nullable() const
{
    // IS_NULLABLE might be NULL.
    return result_.get<string>(17, string());
}

catalog::catalog(connection& conn)
    : conn_(conn)
{
}

catalog::tables catalog::find_tables(
    string const& table,
    string const& type,
    string const& schema,
    string const& catalog)
{
    // Passing a null pointer to a search pattern argument does not
    // constrain the search for that argument; that is, a null pointer and
    // the search pattern % (any characters) are equivalent.
    // However, a zero-length search pattern - that is, a valid pointer to
    // a string of length zero - matches only the empty string ("").
    // See https://msdn.microsoft.com/en-us/library/ms710171.aspx

    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLTables),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(table.empty() ? nullptr : table.c_str()),
        (table.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(type.empty() ? nullptr : type.c_str()),
        (type.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::tables tables(find_result);
    return tables;
}

catalog::procedures
catalog::find_procedures(string const& procedure, string const& schema, string const& catalog)
{
    // Passing a null pointer to a search pattern argument does not
    // constrain the search for that argument; that is, a null pointer and
    // the search pattern % (any characters) are equivalent.
    // However, a zero-length search pattern - that is, a valid pointer to
    // a string of length zero - matches only the empty string ("").
    // See https://msdn.microsoft.com/en-us/library/ms710171.aspx

    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLProcedures),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(procedure.empty() ? nullptr : procedure.c_str()),
        (procedure.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::procedures procedures(find_result);
    return procedures;
}

catalog::procedure_columns catalog::find_procedure_columns(
    string const& column,
    string const& procedure,
    string const& schema,
    string const& catalog)
{
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLProcedureColumns),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(procedure.empty() ? nullptr : procedure.c_str()),
        (procedure.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(column.empty() ? nullptr : column.c_str()),
        (column.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::procedure_columns columns(find_result);
    return columns;
}

catalog::table_privileges
catalog::find_table_privileges(string const& catalog, string const& table, string const& schema)
{
    // Passing a null pointer to a search pattern argument does not
    // constrain the search for that argument; that is, a null pointer and
    // the search pattern % (any characters) are equivalent.
    // However, a zero-length search pattern - that is, a valid pointer to
    // a string of length zero - matches only the empty string ("").
    // See https://msdn.microsoft.com/en-us/library/ms710171.aspx

    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLTablePrivileges),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(table.empty() ? nullptr : table.c_str()),
        (table.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::table_privileges privileges(find_result);
    return privileges;
}

catalog::columns catalog::find_columns(
    string const& column,
    string const& table,
    string const& schema,
    string const& catalog)
{
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLColumns),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(table.empty() ? nullptr : table.c_str()),
        (table.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(column.empty() ? nullptr : column.c_str()),
        (column.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::columns columns(find_result);
    return columns;
}

catalog::primary_keys
catalog::find_primary_keys(string const& table, string const& schema, string const& catalog)
{
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLPrimaryKeys),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)(catalog.empty() ? nullptr : catalog.c_str()),
        (catalog.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(schema.empty() ? nullptr : schema.c_str()),
        (schema.empty() ? 0 : SQL_NTS),
        (NANODBC_SQLCHAR*)(table.empty() ? nullptr : table.c_str()),
        (table.empty() ? 0 : SQL_NTS));
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::primary_keys keys(find_result);
    return keys;
}

std::list<string> catalog::list_catalogs()
{
    // Special case for list of catalogs only:
    // all the other arguments must match empty string (""),
    // otherwise pattern-based lookup is performed returning
    // Cartesian product of catalogs, tables and schemas.
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLTables),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)SQL_ALL_CATALOGS,
        1,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::tables catalogs(find_result);

    std::list<string> names;
    while (catalogs.next())
        names.push_back(catalogs.table_catalog());
    return names;
}

std::list<string> catalog::list_schemas()
{
    // Special case for list of schemas:
    // all the other arguments must match empty string (""),
    // otherwise pattern-based lookup is performed returning
    // Cartesian product of catalogs, tables and schemas.
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLTables),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)SQL_ALL_SCHEMAS,
        1,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::tables schemas(find_result);

    std::list<string> names;
    while (schemas.next())
        names.push_back(schemas.table_schema());
    return names;
}

std::list<string> catalog::list_table_types()
{
    statement stmt(conn_);
    RETCODE rc;
    NANODBC_CALL_RC(
        NANODBC_FUNC(SQLTables),
        rc,
        stmt.native_statement_handle(),
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)NANODBC_TEXT(""),
        0,
        (NANODBC_SQLCHAR*)SQL_ALL_TABLE_TYPES,
        1);
    if (!success(rc))
        NANODBC_THROW_DATABASE_ERROR(stmt.native_statement_handle(), SQL_HANDLE_STMT);

    result find_result(stmt, 1);
    catalog::tables table_types(find_result);

    std::list<string> names;
    while (table_types.next())
        names.push_back(table_types.table_type());
    return names;
}

} // namespace nanodbc

// clang-format off
// 8888888b.                            888 888              8888888888                 888
// 888   Y88b                           888 888              888                        888
// 888    888                           888 888              888                        888
// 888   d88P .d88b.  .d8888b  888  888 888 888888           8888888 888  888  888  .d88888
// 8888888P" d8P  Y8b 88K      888  888 888 888              888     888  888  888 d88" 888
// 888 T88b  88888888 "Y8888b. 888  888 888 888              888     888  888  888 888  888
// 888  T88b Y8b.          X88 Y88b 888 888 Y88b.            888     Y88b 888 d88P Y88b 888
// 888   T88b "Y8888   88888P'  "Y88888 888  "Y888           888      "Y8888888P"   "Y88888
// MARK: Result Fwd -
// clang-format on

namespace nanodbc
{

result::result()
    : impl_()
{
}

result::~result() noexcept {}

result::result(statement stmt, long rowset_size)
    : impl_(new result_impl(std::move(stmt), rowset_size))
{
}

result::result(result&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
{
}

result::result(const result& rhs)
    : impl_(rhs.impl_)
{
}

result& result::operator=(result rhs)
{
    swap(rhs);
    return *this;
}

void result::swap(result& rhs) noexcept
{
    using std::swap;
    swap(impl_, rhs.impl_);
}

void* result::native_statement_handle() const
{
    return impl_->native_statement_handle();
}

long result::rowset_size() const noexcept
{
    return impl_->rowset_size();
}

long result::affected_rows() const
{
    return impl_->affected_rows();
}

bool result::has_affected_rows() const
{
    return impl_->has_affected_rows();
}

long result::rows() const noexcept
{
    return impl_->rows();
}

short result::columns() const
{
    return impl_->columns();
}

bool result::first()
{
    return impl_->first();
}

bool result::last()
{
    return impl_->last();
}

bool result::next()
{
    return impl_->next();
}

#if defined(NANODBC_DO_ASYNC_IMPL)
bool result::async_next(void* event_handle)
{
    return impl_->async_next(event_handle);
}

bool result::complete_next()
{
    return impl_->complete_next();
}
#endif

bool result::prior()
{
    return impl_->prior();
}

bool result::move(long row)
{
    return impl_->move(row);
}

bool result::skip(long rows)
{
    return impl_->skip(rows);
}

unsigned long result::position() const
{
    return impl_->position();
}

bool result::at_end() const noexcept
{
    return impl_->at_end();
}

bool result::is_null(short column) const
{
    return impl_->is_null(column);
}

bool result::is_null(string const& column_name) const
{
    return impl_->is_null(column_name);
}

bool result::is_bound(short column) const
{
    return impl_->is_bound(column);
}

bool result::is_bound(string const& column_name) const
{
    return impl_->is_bound(column_name);
}

short result::column(string const& column_name) const
{
    return impl_->column(column_name);
}

string result::column_name(short column) const
{
    return impl_->column_name(column);
}

long result::column_size(short column) const
{
    return impl_->column_size(column);
}

long result::column_size(string const& column_name) const
{
    return impl_->column_size(column_name);
}

int result::column_decimal_digits(short column) const
{
    return impl_->column_decimal_digits(column);
}

int result::column_decimal_digits(string const& column_name) const
{
    return impl_->column_decimal_digits(column_name);
}

int result::column_datatype(short column) const
{
    return impl_->column_datatype(column);
}

int result::column_datatype(string const& column_name) const
{
    return impl_->column_datatype(column_name);
}

string result::column_datatype_name(short column) const
{
    return impl_->column_datatype_name(column);
}

string result::column_datatype_name(string const& column_name) const
{
    return impl_->column_datatype_name(column_name);
}

int result::column_c_datatype(short column) const
{
    return impl_->column_c_datatype(column);
}

int result::column_c_datatype(string const& column_name) const
{
    return impl_->column_c_datatype(column_name);
}

bool result::next_result()
{
    return impl_->next_result();
}

void result::unbind()
{
    impl_->unbind();
}

void result::unbind(short column)
{
    impl_->unbind(column);
}

void result::unbind(string const& column_name)
{
    impl_->unbind(column_name);
}

template <class T>
void result::get_ref(short column, T& result) const
{
    return impl_->get_ref<T>(column, result);
}

template <class T>
void result::get_ref(short column, T const& fallback, T& result) const
{
    return impl_->get_ref<T>(column, fallback, result);
}

template <class T>
void result::get_ref(string const& column_name, T& result) const
{
    return impl_->get_ref<T>(column_name, result);
}

template <class T>
void result::get_ref(string const& column_name, T const& fallback, T& result) const
{
    return impl_->get_ref<T>(column_name, fallback, result);
}

template <class T>
T result::get(short column) const
{
    return impl_->get<T>(column);
}

template <class T>
T result::get(short column, T const& fallback) const
{
    return impl_->get<T>(column, fallback);
}

template <class T>
T result::get(string const& column_name) const
{
    return impl_->get<T>(column_name);
}

template <class T>
T result::get(string const& column_name, T const& fallback) const
{
    return impl_->get<T>(column_name, fallback);
}

result::operator bool() const
{
    return static_cast<bool>(impl_);
}

// The following are the only supported instantiations of result::get_ref().
template void result::get_ref(short, std::string::value_type&) const;
template void result::get_ref(short, wide_string::value_type&) const;
template void result::get_ref(short, short&) const;
template void result::get_ref(short, unsigned short&) const;
template void result::get_ref(short, int&) const;
template void result::get_ref(short, unsigned int&) const;
template void result::get_ref(short, long int&) const;
template void result::get_ref(short, unsigned long int&) const;
template void result::get_ref(short, long long int&) const;
template void result::get_ref(short, unsigned long long int&) const;
template void result::get_ref(short, float&) const;
template void result::get_ref(short, double&) const;
template void result::get_ref(short, string&) const;
template void result::get_ref(short, date&) const;
template void result::get_ref(short, time&) const;
template void result::get_ref(short, timestamp&) const;
template void result::get_ref(short, std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template void result::get_ref(short, _variant_t&) const;
#endif

template void result::get_ref(string const&, std::string::value_type&) const;
template void result::get_ref(string const&, wide_string::value_type&) const;
template void result::get_ref(string const&, short&) const;
template void result::get_ref(string const&, unsigned short&) const;
template void result::get_ref(string const&, int&) const;
template void result::get_ref(string const&, unsigned int&) const;
template void result::get_ref(string const&, long int&) const;
template void result::get_ref(string const&, unsigned long int&) const;
template void result::get_ref(string const&, long long int&) const;
template void result::get_ref(string const&, unsigned long long int&) const;
template void result::get_ref(string const&, float&) const;
template void result::get_ref(string const&, double&) const;
template void result::get_ref(string const&, string&) const;
template void result::get_ref(string const&, date&) const;
template void result::get_ref(string const&, time&) const;
template void result::get_ref(string const&, timestamp&) const;
template void result::get_ref(string const&, std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template void result::get_ref(string const&, _variant_t&) const;
#endif

#ifdef NANODBC_HAS_STD_OPTIONAL
// The following are the only supported instantiations of result::get() with optional support.
template void result::get_ref(short, std::optional<std::string::value_type>&) const;
template void result::get_ref(short, std::optional<wide_string::value_type>&) const;
template void result::get_ref(short, std::optional<short>&) const;
template void result::get_ref(short, std::optional<unsigned short>&) const;
template void result::get_ref(short, std::optional<int>&) const;
template void result::get_ref(short, std::optional<unsigned int>&) const;
template void result::get_ref(short, std::optional<long int>&) const;
template void result::get_ref(short, std::optional<unsigned long int>&) const;
template void result::get_ref(short, std::optional<long long int>&) const;
template void result::get_ref(short, std::optional<unsigned long long int>&) const;
template void result::get_ref(short, std::optional<float>&) const;
template void result::get_ref(short, std::optional<double>&) const;
template void result::get_ref(short, std::optional<string>&) const;
template void result::get_ref(short, std::optional<date>&) const;
template void result::get_ref(short, std::optional<time>&) const;
template void result::get_ref(short, std::optional<timestamp>&) const;
template void result::get_ref(short, std::optional<std::vector<std::uint8_t>>&) const;
#if defined(_MSC_VER)
template void result::get_ref(short, std::optional<_variant_t>&) const;
#endif

template void result::get_ref(string const&, std::optional<std::string::value_type>&) const;
template void result::get_ref(string const&, std::optional<wide_string::value_type>&) const;
template void result::get_ref(string const&, std::optional<short>&) const;
template void result::get_ref(string const&, std::optional<unsigned short>&) const;
template void result::get_ref(string const&, std::optional<int>&) const;
template void result::get_ref(string const&, std::optional<unsigned int>&) const;
template void result::get_ref(string const&, std::optional<long int>&) const;
template void result::get_ref(string const&, std::optional<unsigned long int>&) const;
template void result::get_ref(string const&, std::optional<long long int>&) const;
template void result::get_ref(string const&, std::optional<unsigned long long int>&) const;
template void result::get_ref(string const&, std::optional<float>&) const;
template void result::get_ref(string const&, std::optional<double>&) const;
template void result::get_ref(string const&, std::optional<string>&) const;
template void result::get_ref(string const&, std::optional<date>&) const;
template void result::get_ref(string const&, std::optional<time>&) const;
template void result::get_ref(string const&, std::optional<timestamp>&) const;
template void result::get_ref(string const&, std::optional<std::vector<std::uint8_t>>&) const;
#if defined(_MSC_VER)
template void result::get_ref(short, std::optional<_variant_t>&) const;
#endif
#endif

// The following are the only supported instantiations of result::get_ref() with fallback.
template void
result::get_ref(short, const std::string::value_type&, std::string::value_type&) const;
template void
result::get_ref(short, const wide_string::value_type&, wide_string::value_type&) const;
template void result::get_ref(short, const short&, short&) const;
template void result::get_ref(short, const unsigned short&, unsigned short&) const;
template void result::get_ref(short, const int&, int&) const;
template void result::get_ref(short, const unsigned int&, unsigned int&) const;
template void result::get_ref(short, const long int&, long int&) const;
template void result::get_ref(short, const unsigned long int&, unsigned long int&) const;
template void result::get_ref(short, const long long int&, long long int&) const;
template void result::get_ref(short, const unsigned long long int&, unsigned long long int&) const;
template void result::get_ref(short, const float&, float&) const;
template void result::get_ref(short, const double&, double&) const;
template void result::get_ref(short, string const&, string&) const;
template void result::get_ref(short, const date&, date&) const;
template void result::get_ref(short, const time&, time&) const;
template void result::get_ref(short, const timestamp&, timestamp&) const;
template void
result::get_ref(short, const std::vector<std::uint8_t>&, std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template void result::get_ref(short, _variant_t const&, _variant_t&) const;
#endif

template void
result::get_ref(string const&, const std::string::value_type&, std::string::value_type&) const;
template void
result::get_ref(string const&, const wide_string::value_type&, wide_string::value_type&) const;
template void result::get_ref(string const&, const short&, short&) const;
template void result::get_ref(string const&, const unsigned short&, unsigned short&) const;
template void result::get_ref(string const&, const int&, int&) const;
template void result::get_ref(string const&, const unsigned int&, unsigned int&) const;
template void result::get_ref(string const&, const long int&, long int&) const;
template void result::get_ref(string const&, const unsigned long int&, unsigned long int&) const;
template void result::get_ref(string const&, const long long int&, long long int&) const;
template void
result::get_ref(string const&, const unsigned long long int&, unsigned long long int&) const;
template void result::get_ref(string const&, const float&, float&) const;
template void result::get_ref(string const&, const double&, double&) const;
template void result::get_ref(string const&, std::string const&, std::string&) const;
template void result::get_ref(string const&, const wide_string&, wide_string&) const;
template void result::get_ref(string const&, const date&, date&) const;
template void result::get_ref(string const&, const time&, time&) const;
template void result::get_ref(string const&, const timestamp&, timestamp&) const;
template void
result::get_ref(string const&, const std::vector<std::uint8_t>&, std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template void result::get_ref(string const&, _variant_t const&, _variant_t&) const;
#endif

// The following are the only supported instantiations of result::get().
template std::string::value_type result::get(short) const;
template wide_string::value_type result::get(short) const;
template short result::get(short) const;
template unsigned short result::get(short) const;
template int result::get(short) const;
template unsigned int result::get(short) const;
template long int result::get(short) const;
template unsigned long int result::get(short) const;
template long long int result::get(short) const;
template unsigned long long int result::get(short) const;
template float result::get(short) const;
template double result::get(short) const;
template std::string result::get(short) const;
template wide_string result::get(short) const;
template date result::get(short) const;
template time result::get(short) const;
template timestamp result::get(short) const;
template std::vector<std::uint8_t> result::get(short) const;
#if defined(_MSC_VER)
template _variant_t result::get(short) const;
#endif

template std::string::value_type result::get(string const&) const;
template wide_string::value_type result::get(string const&) const;
template short result::get(string const&) const;
template unsigned short result::get(string const&) const;
template int result::get(string const&) const;
template unsigned int result::get(string const&) const;
template long int result::get(string const&) const;
template unsigned long int result::get(string const&) const;
template long long int result::get(string const&) const;
template unsigned long long int result::get(string const&) const;
template float result::get(string const&) const;
template double result::get(string const&) const;
template std::string result::get(string const&) const;
template wide_string result::get(string const&) const;
template date result::get(string const&) const;
template time result::get(string const&) const;
template timestamp result::get(string const&) const;
template std::vector<std::uint8_t> result::get(string const&) const;
#if defined(_MSC_VER)
template _variant_t result::get(string const&) const;
#endif

#ifdef NANODBC_HAS_STD_OPTIONAL
// The following are the only supported instantiations of result::get() with optional support.
template std::optional<std::string::value_type> result::get(short) const;
template std::optional<wide_string::value_type> result::get(short) const;
template std::optional<short> result::get(short) const;
template std::optional<unsigned short> result::get(short) const;
template std::optional<int> result::get(short) const;
template std::optional<unsigned int> result::get(short) const;
template std::optional<long int> result::get(short) const;
template std::optional<unsigned long int> result::get(short) const;
template std::optional<long long int> result::get(short) const;
template std::optional<unsigned long long int> result::get(short) const;
template std::optional<float> result::get(short) const;
template std::optional<double> result::get(short) const;
template std::optional<std::string> result::get(short) const;
template std::optional<wide_string> result::get(short) const;
template std::optional<date> result::get(short) const;
template std::optional<time> result::get(short) const;
template std::optional<timestamp> result::get(short) const;
template std::optional<std::vector<std::uint8_t>> result::get(short) const;
#if defined(_MSC_VER)
template std::optional<_variant_t> result::get(short) const;
#endif

template std::optional<std::string::value_type> result::get(string const&) const;
template std::optional<wide_string::value_type> result::get(string const&) const;
template std::optional<short> result::get(string const&) const;
template std::optional<unsigned short> result::get(string const&) const;
template std::optional<int> result::get(string const&) const;
template std::optional<unsigned int> result::get(string const&) const;
template std::optional<long int> result::get(string const&) const;
template std::optional<unsigned long int> result::get(string const&) const;
template std::optional<long long int> result::get(string const&) const;
template std::optional<unsigned long long int> result::get(string const&) const;
template std::optional<float> result::get(string const&) const;
template std::optional<double> result::get(string const&) const;
template std::optional<std::string> result::get(string const&) const;
template std::optional<wide_string> result::get(string const&) const;
template std::optional<date> result::get(string const&) const;
template std::optional<time> result::get(string const&) const;
template std::optional<timestamp> result::get(string const&) const;
template std::optional<std::vector<std::uint8_t>> result::get(string const&) const;
#if defined(_MSC_VER)
template std::optional<_variant_t> result::get(string const&) const;
#endif
#endif

// The following are the only supported instantiations of result::get() with fallback.
template std::string::value_type result::get(short, const std::string::value_type&) const;
template wide_string::value_type result::get(short, const wide_string::value_type&) const;
template short result::get(short, const short&) const;
template unsigned short result::get(short, const unsigned short&) const;
template int result::get(short, const int&) const;
template unsigned int result::get(short, const unsigned int&) const;
template long int result::get(short, const long int&) const;
template unsigned long int result::get(short, const unsigned long int&) const;
template long long int result::get(short, const long long int&) const;
template unsigned long long int result::get(short, const unsigned long long int&) const;
template float result::get(short, const float&) const;
template double result::get(short, const double&) const;
template std::string result::get(short, std::string const&) const;
template wide_string result::get(short, const wide_string&) const;
template date result::get(short, const date&) const;
template time result::get(short, const time&) const;
template timestamp result::get(short, const timestamp&) const;
template std::vector<std::uint8_t> result::get(short, const std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template _variant_t result::get(short, _variant_t const&) const;
#endif

template std::string::value_type result::get(string const&, const std::string::value_type&) const;
template wide_string::value_type result::get(string const&, const wide_string::value_type&) const;
template short result::get(string const&, const short&) const;
template unsigned short result::get(string const&, const unsigned short&) const;
template int result::get(string const&, const int&) const;
template unsigned int result::get(string const&, const unsigned int&) const;
template long int result::get(string const&, const long int&) const;
template unsigned long int result::get(string const&, const unsigned long int&) const;
template long long int result::get(string const&, const long long int&) const;
template unsigned long long int result::get(string const&, const unsigned long long int&) const;
template float result::get(string const&, const float&) const;
template double result::get(string const&, const double&) const;
template std::string result::get(string const&, std::string const&) const;
template wide_string result::get(string const&, const wide_string&) const;
template date result::get(string const&, const date&) const;
template time result::get(string const&, const time&) const;
template timestamp result::get(string const&, const timestamp&) const;
template std::vector<std::uint8_t>
result::get(string const&, const std::vector<std::uint8_t>&) const;
#if defined(_MSC_VER)
template _variant_t result::get(string const&, _variant_t const&) const;
#endif

} // namespace nanodbc
#endif // NANODBC_DISABLE_NANODBC_NAMESPACE_FOR_INTERNAL_TESTS

#undef NANODBC_THROW_DATABASE_ERROR
#undef NANODBC_STRINGIZE
#undef NANODBC_STRINGIZE_I
#undef NANODBC_CALL_RC
#undef NANODBC_CALL

#endif // DOXYGEN
