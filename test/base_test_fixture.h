#ifndef NANODBC_TEST_BASE_FIXTURE_H
#define NANODBC_TEST_BASE_FIXTURE_H

#include <nanodbc/nanodbc.h>

#include <cassert>
#include <iostream>
#include <random>
#include <tuple>

#if defined(_MSC_VER) && _MSC_VER <= 1800
// These versions of Visual C++ do not yet support noexcept or override.
#define NANODBC_NOEXCEPT
#define NANODBC_OVERRIDE
#else
#define NANODBC_NOEXCEPT noexcept
#define NANODBC_OVERRIDE override
#endif

#ifdef NANODBC_ENABLE_BOOST
#include <boost/locale/encoding_utf.hpp>
#else
#include <codecvt>
#endif

#ifdef _WIN32
// needs to be included above sql.h for windows
#ifndef __MINGW32__
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

struct TestConfig
{
    nanodbc::string_type get_connection_string() const
    {
#ifdef NANODBC_ENABLE_UNICODE
#ifdef NANODBC_ENABLE_BOOST
        using boost::locale::conv::utf_to_utf;
        return utf_to_utf<char16_t>(
            connection_string_.c_str(), connection_string_.c_str() + connection_string_.size());
// Workaround for confirmed bug in VS2015 and VS2017 too
// See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
#elif defined(_MSC_VER) && (_MSC_VER >= 1900)
        auto s = std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t>().from_bytes(
            connection_string_);
        auto p = reinterpret_cast<char16_t const*>(s.data());
        return nanodbc::string_type(p, p + s.size());
#else
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(
            connection_string_);
#endif
#else
        return connection_string_;
#endif
    }

    std::string connection_string_;
};
extern TestConfig cfg;

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
        : connection_string_(cfg.get_connection_string())
    {
        // Connection string not specified in command line, try environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("TEST_NANODBC_CONNSTR");
    }

    virtual ~base_test_fixture() NANODBC_NOEXCEPT {}

    // Utilities

    nanodbc::string_type connection_string_;

    database_vendor vendor_ = database_vendor::unknown;

    database_vendor get_vendor(nanodbc::string_type const& dbms)
    {
        REQUIRE(!dbms.empty());
        if (contains_string(dbms, NANODBC_TEXT("Oracle")))
            return database_vendor::oracle;
        else if (contains_string(dbms, NANODBC_TEXT("SQLite")))
            return database_vendor::sqlite;
        else if (contains_string(dbms, NANODBC_TEXT("PostgreSQL")))
            return database_vendor::postgresql;
        else if (contains_string(dbms, NANODBC_TEXT("MySQL")))
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

    nanodbc::string_type get_binary_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::sqlite:
        case database_vendor::mysql:
            return NANODBC_TEXT("blob");
        case database_vendor::postgresql:
            return NANODBC_TEXT("bytea");
        default:
            return NANODBC_TEXT("varbinary"); // Oracle, MySQL, SQL Server,...standard type?
        }
    }

    nanodbc::string_type get_text_type_name()
    {
        switch (vendor_)
        {
        case database_vendor::vertica:
            return NANODBC_TEXT("long varchar");
        default:
            return NANODBC_TEXT("text"); // Oracle, MySQL, SQL Server,...standard type?
        }
    }

    nanodbc::string_type get_primary_key_name(nanodbc::string_type const& assumed)
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

    void check_data_type_size(nanodbc::string_type const& name, int column_size, short radix = -1)
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
                REQUIRE(column_size >= 15);
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
        return connection;
    }

    nanodbc::string_type connection_string_parameter(nanodbc::string_type const& keyword)
    {
        // Find given keyword in the semi-colon-separated keyword=value pairs
        // of connection string and return its value, strippng `{` and `}` wrappers.
        if (connection_string_.empty())
            return nanodbc::string_type();

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
                    beg_value++;
                auto end_value = pair_end;
                if (*(end_value - 1) == NANODBC_TEXT('}'))
                    end_value--;

                return {beg_value, end_value};
            }

            beg = pair_end + 1;
        }
        return nanodbc::string_type();
    }

    static void check_rows_equal(nanodbc::result results, int rows)
    {
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == rows);
    }

    static std::string to_hex_string(std::vector<std::uint8_t> const& bytes)
    {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::uppercase;
        for (auto const& b : bytes)
            ss << std::setw(2) << static_cast<int>(b);
        return ss.str();
    }

    nanodbc::string_type get_env(char const* var) const
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
            return nanodbc::string_type();
        value = env_value;
#endif

#ifdef NANODBC_ENABLE_UNICODE
#ifdef NANODBC_ENABLE_BOOST
        using boost::locale::conv::utf_to_utf;
        return utf_to_utf<char16_t>(value.c_str(), value.c_str() + value.size());
// Workaround for confirmed bug in VS2015 and VS2017 too
// See: https://connect.microsoft.com/VisualStudio/Feedback/Details/1403302
#elif defined(_MSC_VER) && (_MSC_VER >= 1900)
        auto s =
            std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t>().from_bytes(value);
        auto p = reinterpret_cast<char16_t const*>(s.data());
        return nanodbc::string_type(p, p + s.size());
#else
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(
            value);
#endif
#else
        return value;
#endif
    }

    bool contains_string(nanodbc::string_type const& str, nanodbc::string_type const& sub)
    {
        if (str.empty() || sub.empty())
            return false;

        return str.find(sub) != nanodbc::string_type::npos;
    }

    bool iequals_string(
        nanodbc::string_type const& lhs,
        nanodbc::string_type const& rhs,
        std::locale const& loc = std::locale())
    {
        struct is_iequal
        {
            using char_type = typename nanodbc::string_type::value_type;

            is_iequal(std::locale const& loc)
                : loc_(loc)
            {
            }

            bool operator()(char_type const& lhs, char_type const& rhs)
            {
                // FIXME: This is ugly, but ctype<char16_t> and ctype<char32_t> specializations
                // are not mandatory according to the C++11, only for char and wchar_t are.
                // So, use the one with bigger capacity of the two.
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
        nanodbc::string_type const& name,
        nanodbc::string_type def) const
    {
        if (def.front() != NANODBC_TEXT('('))
            def.insert(0, 1, NANODBC_TEXT('('));

        if (def.back() != NANODBC_TEXT(')'))
            def.push_back(NANODBC_TEXT(')'));

        nanodbc::string_type sql(NANODBC_TEXT("CREATE TABLE "));
        sql += name;
        sql += NANODBC_TEXT(" ");
        sql += def;
        sql += NANODBC_TEXT(';');

        drop_table(connection, name);
        execute(connection, sql);
    }

    virtual void drop_table(nanodbc::connection& connection, nanodbc::string_type const& name) const
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
            execute(connection, NANODBC_TEXT("DROP TABLE ") + name + NANODBC_TEXT(";"));
        }
    }
};

#endif // NANODBC_TEST_BASE_FIXTURE_H
