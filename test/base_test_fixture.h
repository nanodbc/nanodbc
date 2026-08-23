#ifndef NANODBC_TEST_BASE_FIXTURE_H
#define NANODBC_TEST_BASE_FIXTURE_H

#include "catch/catch_amalgamated.hpp"

#include <nanodbc/nanodbc.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <locale>
#include <random>
#include <sstream>

#ifdef NANODBC_ENABLE_BOOST
#include <boost/locale/encoding_utf.hpp>
#endif

#include <stdexcept>
#include <type_traits>

#ifdef _WIN32
// needs to be included above sql.h for windows
#if !defined(__MINGW32__) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <locale>
#include <sql.h>
#include <sqlext.h>

namespace nanodbc
{
namespace test
{

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

    int extra = 0;
    char32_t cp = 0;
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
#ifdef NANODBC_ENABLE_BOOST
    using boost::locale::conv::utf_to_utf;
    return utf_to_utf<nanodbc::string::value_type>(in.c_str(), in.c_str() + in.size());
#else
    nanodbc::string out;
    auto const* beg = in.data();
    auto const* const end = beg + in.size();
    while (beg != end)
        detail::append_as_wide(detail::next_utf8_code_point(beg, end), out);
    return out;
#endif
}

inline std::string convert(nanodbc::string const& in)
{
    static_assert(sizeof(nanodbc::string::value_type) > 1, "string must be wide");
#ifdef NANODBC_ENABLE_BOOST
    using boost::locale::conv::utf_to_utf;
    return utf_to_utf<char>(in.c_str(), in.c_str() + in.size());
#else
    std::string out;
    auto const* beg = in.data();
    auto const* const end = beg + in.size();
    while (beg != end)
        detail::append_as_utf8(detail::next_wide_code_point(beg, end), out);
    return out;
#endif
}
#else
inline nanodbc::string convert(std::string const& in)
{
    return in;
}
#endif

struct Config
{
    nanodbc::string get_connection_string() const { return convert(connection_string_); }

    std::string connection_string_;
    std::string data_path_;
    std::string test_; // if set, itis test name, pattern or tags
};

// Custom matcher for Catch to use with REQUIRE_THAT(a, IsAnyOf({a, b, c}));
class IntAnyOf : public Catch::Matchers::MatcherBase<int>
{
    std::vector<int> m_values;

public:
    IntAnyOf(std::initializer_list<int> v)
        : m_values(v)
    {
    }

    // Performs the test for this matcher
    virtual bool match(int const& i) const override
    {
        return std::any_of(m_values.begin(), m_values.end(), [&i](int v) { return v == i; });
    }

    // Produces a string describing what this matcher does. It should
    // include any provided data (the begin/ end in this case) and
    // be written as if it were stating a fact (in the output it will be
    // preceded by the value under test).
    virtual std::string describe() const override
    {
        std::ostringstream ss;
        ss << "is not member of values [";
        for (auto& v : m_values)
            ss << v << ',';
        ss << ']';
        return ss.str();
    }
};

// The builder function
inline IntAnyOf IsAnyOf(std::initializer_list<int> v)
{
    return IntAnyOf(std::move(v));
}
} // namespace test
} // namespace nanodbc

extern nanodbc::test::Config cfg;

struct base_test_fixture
{
    // Database vendor
    // Determine DBMS-specific features, properties and values
    // NOTE: If handling DBMS-specific features become overly complicated,
    //       we may decided to remove such features from the tests.
    enum class database_vendor
    {
        unknown,
        oracle,
        sqlite,
        postgresql,
        mysql,
        sqlserver,
        vertica
    };

    base_test_fixture()
        : connection_string_{cfg.get_connection_string()}
        , data_path_(cfg.data_path_)
    {
        // Connection string not specified in command line, try environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR");

        // Path to data folder with data files used in some tests
        if (data_path_.empty())
            data_path_ = nanodbc::test::convert(get_env("NANODBC_TEST_DATADIR"));
    }

    virtual ~base_test_fixture() noexcept {}

    // Utilities

    nanodbc::string connection_string_;
    std::string data_path_;

    database_vendor vendor_ = database_vendor::unknown;
    // MariaDB is reached through the same tests as MySQL, but its own driver answers some
    // catalog questions differently, so the two are told apart where that matters.
    bool mariadb_ = false;

    database_vendor get_vendor(nanodbc::string const& dbms)
    {
        REQUIRE(!dbms.empty());
        if (contains_string(dbms, NANODBC_TEXT("Oracle")))
            return database_vendor::oracle;
        else if (contains_string(dbms, NANODBC_TEXT("SQLite")))
            return database_vendor::sqlite;
        else if (contains_string(dbms, NANODBC_TEXT("PostgreSQL")))
            return database_vendor::postgresql;
        else if (
            contains_string(dbms, NANODBC_TEXT("MySQL")) ||
            contains_string(dbms, NANODBC_TEXT("MariaDB")))
            return database_vendor::mysql;
        else if (
            contains_string(dbms, NANODBC_TEXT("SQLServer")) ||
            contains_string(dbms, NANODBC_TEXT("SQL Server")))
            return database_vendor::sqlserver;
        else if (contains_string(dbms, NANODBC_TEXT("Vertica")))
            return database_vendor::vertica;
        else
            return database_vendor::unknown;
    }

    nanodbc::string get_binary_type_name(int size = 0)
    {
        nanodbc::string s;
        {
            std::string t;
            if (size)
                t = "(" + std::to_string(size) + ")";
            s = nanodbc::test::convert(t);
        }

        switch (vendor_)
        {
        case database_vendor::sqlite:
        case database_vendor::mysql:
            return NANODBC_TEXT("blob") + s;
        case database_vendor::postgresql:
            return NANODBC_TEXT("bytea");
        case database_vendor::oracle:
            // Oracle spells a bounded binary raw, which holds 2000 bytes at most, and
            // anything longer than that a blob.
            return size > 0 && size <= 2000 ? NANODBC_TEXT("raw") + s : NANODBC_TEXT("blob");
        default:
            return NANODBC_TEXT("varbinary") + s;
        }
    }

    nanodbc::string get_bool_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::sqlserver:
            return NANODBC_TEXT("bit");
        default:
            return NANODBC_TEXT("boolean");
        }
    }

    // Oracle has no bigint; its numbers carry a precision instead.
    nanodbc::string get_bigint_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::oracle:
            return NANODBC_TEXT("number(19)");
        default:
            return NANODBC_TEXT("bigint");
        }
    }

    nanodbc::string get_timestamp_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::mysql:
        case database_vendor::sqlserver:
            return NANODBC_TEXT("datetime");
        default:
            return NANODBC_TEXT("timestamp");
        }
    }

    nanodbc::string get_text_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::vertica:
            return NANODBC_TEXT("long varchar");
        case database_vendor::oracle:
            // Oracle has no text; a character column with no bound is a clob.
            return NANODBC_TEXT("clob");
        default:
            return NANODBC_TEXT("text");
        }
    }

    // SQLite and MySQL spell the aggregate group_concat, and disagree on how the
    // separator is given; everyone else spells it string_agg.
    nanodbc::string
    get_string_agg_expression(nanodbc::string const& column, nanodbc::string const& separator)
    {
        switch (vendor_)
        {
        case database_vendor::sqlite:
            return NANODBC_TEXT("group_concat(") + column + NANODBC_TEXT(", '") + separator +
                   NANODBC_TEXT("')");
        case database_vendor::mysql:
            return NANODBC_TEXT("group_concat(") + column + NANODBC_TEXT(" separator '") +
                   separator + NANODBC_TEXT("')");
        case database_vendor::oracle:
            // Oracle spells it listagg, and wants the order stated.
            return NANODBC_TEXT("listagg(") + column + NANODBC_TEXT(", '") + separator +
                   NANODBC_TEXT("') within group (order by ") + column + NANODBC_TEXT(")");
        default:
            return NANODBC_TEXT("string_agg(") + column + NANODBC_TEXT(", '") + separator +
                   NANODBC_TEXT("')");
        }
    }

    // An unquoted identifier folds to upper case on Oracle, which is what the standard
    // asks for, so a name written lower case in a query comes back upper case from the
    // catalog and has to be looked up that way.
    nanodbc::string as_identifier(nanodbc::string const& name) const
    {
        if (vendor_ != database_vendor::oracle)
            return name;

        nanodbc::string folded;
        folded.reserve(name.size());
        for (auto const c : name)
        {
            folded.push_back(
                c >= static_cast<nanodbc::string::value_type>('a') &&
                        c <= static_cast<nanodbc::string::value_type>('z')
                    ? static_cast<nanodbc::string::value_type>(c - ('a' - 'A'))
                    : c);
        }
        return folded;
    }

    nanodbc::string get_primary_key_name(nanodbc::string const& assumed)
    {
        switch (vendor_)
        {
        case database_vendor::mysql:
            return NANODBC_TEXT("PRIMARY"); // MySQL: The name of a PRIMARY KEY is always PRIMARY
        case database_vendor::sqlite:
            return NANODBC_TEXT(""); // NOTE: SQLite seem to have no support for named PK constraint
        default:
            return assumed;
        }
    }

    void check_data_type_size(nanodbc::string const& name, long column_size, short radix = -1)
    {
        if (name == NANODBC_TEXT("float"))
        {
            if (radix == 2)
            {
                REQUIRE(column_size == 53); // total number of bits allowed
            }
            else if (radix == 10)
            {
                // total number of digits allowed

                // NOTE: Some variations have been observed:
                //
                // - Windows 64-bit + nanodbc 64-bit build + psqlODBC 9.?.? x64 connected to
                //   PostgreSQL 9.3 on Windows x64 (AppVeyor)
                if (mariadb_)
                {
                    // MariaDB's own driver reports the digits of a single precision float.
                    REQUIRE(column_size >= 7);
                }
                else if (vendor_ == database_vendor::mysql)
                {
                    // MySQL Connector 8.1 reports different value than MySQL Connector 5.3
                    REQUIRE(column_size >= 12);
                }
                else
                {
                    REQUIRE(column_size >= 15);
                }
                // - Windows x64 + nanodbc 64-bit build + psqlODBC 9.3.5 x64 connected to
                //   PostgreSQL 9.5 on Ubuntu 15.10 x64 (Vagrant)
                // - Ubuntu 12.04 x64 + nanodbc 64-bit build + psqlODBC 9.3.5 x64 connected to
                //   PostgreSQL 9.1 on Ubuntu 12.04 x64 (Travsi CI)
                REQUIRE(column_size <= 17);
            }
            else
            {
                ; // driver says, not applicable
            }
        }
        else if (name == NANODBC_TEXT("text"))
        {
            REQUIRE(
                // MySQL
                (column_size == 2147483647 || column_size == 65535 ||
                 // PostgreSQL uses MaxLongVarcharSize=8190, which is configurable in odbc.ini
                 column_size == 8190 ||
                 // SQLite
                 column_size == 0));
        }
        else if (name == NANODBC_TEXT("long varchar"))
        {
            REQUIRE(column_size > 0); // Vertica
        }
    }

    nanodbc::connection connect()
    {
        nanodbc::connection connection(connection_string_);
        REQUIRE(connection.connected());
        vendor_ = get_vendor(connection.dbms_name());
        mariadb_ = contains_string(connection.dbms_name(), NANODBC_TEXT("MariaDB"));
        return connection;
    }

    nanodbc::string connection_string_parameter(nanodbc::string const& keyword)
    {
        // Find given keyword in the semi-colon-separated keyword=value pairs
        // of connection string and return its value, strippng `{` and `}` wrappers.
        if (connection_string_.empty())
            return nanodbc::string();

        auto beg = connection_string_.begin();
        auto const end = connection_string_.end();
        auto pair_end = end;
        while ((pair_end = std::find(beg, end, NANODBC_TEXT(';'))) != end)
        {
            auto const eq_pos = std::find(beg, pair_end, NANODBC_TEXT('='));
            if (eq_pos == end)
                break;

            if (iequals_string(keyword, {beg, eq_pos}))
            {
                auto beg_value = eq_pos + 1;
                if (*beg_value == NANODBC_TEXT('{'))
                    ++beg_value;
                auto end_value = pair_end;
                if (*(end_value - 1) == NANODBC_TEXT('}'))
                    --end_value;

                return {beg_value, end_value};
            }

            beg = pair_end + 1;
        }
        return nanodbc::string();
    }

    static void check_rows_equal(nanodbc::result results, int rows)
    {
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == rows);
    }

    static auto from_hex(std::string const& hex) -> std::vector<std::uint8_t>
    {
        if (hex.empty() || 0 != hex.size() % 2)
            throw std::runtime_error("invalid lenght of hex string");

        std::string::size_type const nchars = 2;
        std::string::size_type const nbytes = hex.size() / nchars;
        std::vector<std::uint8_t> bytes(nbytes);
        for (std::string::size_type i = 0; i < nbytes; ++i)
        {
            std::istringstream iss(hex.substr(i * nchars, nchars));
            unsigned int n(0);
            if (!(iss >> std::hex >> n))
                throw std::runtime_error("hex to binary failed");
            bytes[i] = static_cast<std::uint8_t>(n);
        }
        return bytes;
    }

    static auto to_hex(std::vector<std::uint8_t> const& bytes) -> std::string
    {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::uppercase;
        for (auto const& b : bytes)
            ss << std::setw(2) << static_cast<int>(b);
        return ss.str();
    }

    static auto read_text_file(std::string const& filename) -> std::string
    {
        std::ifstream infile;
        infile.open(filename);
        std::string buffer;
        infile >> buffer;
        if (buffer.empty())
            return {};

        auto beg = buffer.begin();
        while (*beg == ' ' || *beg == '\0')
            ++beg;
        auto end = buffer.end() - 1;
        while (*end == ' ' || *end == '\0')
            --end;
        return {beg, end + 1};
    }

    auto get_data_path(std::string const& leaf) -> std::string
    {
#ifdef _WIN32
#define NANODBC_SEP '\\'
#else
#define NANODBC_SEP '/'
#endif
        return data_path_ + NANODBC_SEP + leaf;

#undef NANODBC_SEP
    }

    nanodbc::string get_env(char const* var) const
    {
        char* env_value = nullptr;
        std::string value;
#ifdef _MSC_VER
        std::size_t env_len(0);
        errno_t err = _dupenv_s(&env_value, &env_len, var);
        if (!err && env_value)
        {
            value = env_value;
            std::free(env_value);
        }
#else
        env_value = std::getenv(var);
        if (!env_value)
            return nanodbc::string();
        value = env_value;
#endif
#ifdef NANODBC_ENABLE_UNICODE
        return nanodbc::test::convert(value);
#else
        return value;
#endif
    }

    bool contains_string(nanodbc::string const& str, nanodbc::string const& sub)
    {
        if (str.empty() || sub.empty())
            return false;

        return str.find(sub) != nanodbc::string::npos;
    }

    bool iequals_string(
        nanodbc::string const& lhs,
        nanodbc::string const& rhs,
        std::locale const& loc = std::locale())
    {
        struct is_iequal
        {
            using char_type = typename nanodbc::string::value_type;

            explicit is_iequal(std::locale const& loc)
                : loc_(loc)
            {
            }

            bool operator()(char_type const& lhs, char_type const& rhs)
            {
                // Only ctype<char> and ctype<wchar_t> are required to exist, so char16_t
                // and char32_t are folded through the wider of the two.
                return std::toupper<wchar_t>(lhs, loc_) == std::toupper<wchar_t>(rhs, loc_);
            }

        private:
            std::locale loc_;
        };

        if (lhs.length() == rhs.length())
        {
            return std::equal(rhs.cbegin(), rhs.cend(), lhs.begin(), is_iequal(loc));
        }
        else
        {
            return false;
        }
    }

    // `name` is a table name.
    // `def` is a comma separated column definitions, trailing '(' and ')' are optional.
    void create_table(
        nanodbc::connection& connection,
        nanodbc::string const& name,
        nanodbc::string def,
        bool create_view = false) const
    {
        if (!create_view) // i.e. SQLite does not like braces in CREATE VIEW x AS (SELECT...)
        {
            if (def.front() != NANODBC_TEXT('('))
                def.insert(0, 1, NANODBC_TEXT('('));

            if (def.back() != NANODBC_TEXT(')'))
                def.push_back(NANODBC_TEXT(')'));
        }

        nanodbc::string sql(NANODBC_TEXT("CREATE TABLE "));
        if (create_view)
            sql = NANODBC_TEXT("CREATE VIEW ");
        sql += name;
        if (create_view)
            sql += NANODBC_TEXT(" AS ");
        sql += NANODBC_TEXT(" ");
        sql += def;
        sql += NANODBC_TEXT(';');

        drop_table(connection, name, create_view);
        execute(connection, sql);
    }

    virtual void drop_table(
        nanodbc::connection& connection,
        nanodbc::string const& name,
        bool drop_view = false) const
    {
        bool table_exists = true;
        try
        {
            // create empty result set as a poor man's portable "IF EXISTS" test
            nanodbc::result results = execute(
                connection, NANODBC_TEXT("SELECT * FROM ") + name + NANODBC_TEXT(" WHERE 0=1;"));
        }
        catch (...)
        {
            table_exists = false;
        }

        if (table_exists)
        {
            if (drop_view)
                execute(connection, NANODBC_TEXT("DROP VIEW ") + name + NANODBC_TEXT(";"));
            else
                execute(connection, NANODBC_TEXT("DROP TABLE ") + name + NANODBC_TEXT(";"));
        }
    }

    // returns random string [min_size, max_size]
    template <class T, typename = nanodbc::enable_if_string<T>>
    T create_random_string(size_t min_size, size_t max_size)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist_size(min_size, max_size);
        std::uniform_int_distribution<size_t> dist_alpha(0, 25);

        T result;
        result.resize(dist_size(gen));

        for (auto& dst : result)
        {
            // set 'A' to 'Z'
            dst = static_cast<typename T::value_type>('A') +
                  static_cast<typename T::value_type>(dist_alpha(gen));
        }

        return result;
    }

    // returns random binary [min_size, max_size]
    std::vector<uint8_t> create_random_binary(size_t min_size, size_t max_size)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist_size(min_size, max_size);
        std::uniform_int_distribution<size_t> dist_bin(0, 255);

        std::vector<uint8_t> result;
        result.resize(dist_size(gen));

        for (auto& dst : result)
        {
            // set 0x00 to 0xFF
            dst = static_cast<uint8_t>(dist_bin(gen));
        }

        return result;
    }
};

#endif // NANODBC_TEST_BASE_FIXTURE_H
