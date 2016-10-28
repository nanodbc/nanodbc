#ifndef NANODBC_TEST_BASE_TEST_FIXTURE_H
#define NANODBC_TEST_BASE_TEST_FIXTURE_H

#include <iostream>
#include "nanodbc.h"
#include <cassert>
#include <tuple>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // conversion from 'T1' to 'T2' possible loss of data
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
// These versions of Visual C++ do not yet support noexcept or override.
#define NANODBC_NOEXCEPT
#define NANODBC_OVERRIDE
#else
#define NANODBC_NOEXCEPT noexcept
#define NANODBC_OVERRIDE override
#endif

#ifdef NANODBC_USE_BOOST_CONVERT
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
#ifdef NANODBC_USE_UNICODE
#ifdef NANODBC_USE_BOOST_CONVERT
        using boost::locale::conv::utf_to_utf;
        return utf_to_utf<char16_t>(
            connection_string_.c_str(), connection_string_.c_str() + connection_string_.size());
// Workaround for confirmed bug in VS2015.
// See: https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481
#elif defined(_MSC_VER) && (_MSC_VER == 1900)
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

    // To invoke a unit test over all integral types, use:
    //
    typedef std::tuple<
        nanodbc::string_type::value_type,
        short,
        unsigned short,
        int32_t,
        uint32_t,
        int64_t,
        uint64_t,
        float,
        double>
        integral_test_types;

    base_test_fixture()
        : connection_string_(cfg.get_connection_string())
    {
        // Connection string not specified in command line, try environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR");
    }

    base_test_fixture(const nanodbc::string_type& connection_string)
        : connection_string_(connection_string)
    {
    }

    virtual ~base_test_fixture() NANODBC_NOEXCEPT {}

    // Utilities

    nanodbc::string_type connection_string_;

    database_vendor vendor_ = database_vendor::unknown;

    database_vendor get_vendor(const nanodbc::string_type& dbms)
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
        else if (contains_string(dbms, NANODBC_TEXT("SQLServer")))
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

    nanodbc::string_type get_primary_key_name(const nanodbc::string_type& assumed)
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

    void check_data_type_size(const nanodbc::string_type& name, int column_size, short radix = -1)
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

    nanodbc::string_type get_env(const char* var) const
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

#ifdef NANODBC_USE_UNICODE
#ifdef NANODBC_USE_BOOST_CONVERT
        using boost::locale::conv::utf_to_utf;
        return utf_to_utf<char16_t>(value.c_str(), value.c_str() + value.size());
// Workaround for confirmed bug in VS2015.
// See: https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481
#elif defined(_MSC_VER) && (_MSC_VER == 1900)
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

    // Test Cases

    void blob_test()
    {
        nanodbc::string_type s = NANODBC_TEXT(
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
            "BBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
            "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
            "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
            "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFFFFFF"
            "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFGGGGGGGGG"
            "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"
            "GGGHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
            "HHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"
            "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ"
            "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
            "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
            "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLMMMMMMMMMMMMMMMMMMM"
            "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
            "OOOOOOOOOOOOOOOOOOOPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"
            "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ"
            "QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
            "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
            "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTTTTTTTT"
            "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTUUUU"
            "UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU"
            "UUUUUUUUUVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
            "VVVVVVVVVVVVVVVVVVVVVVWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
            "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
            "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"
            "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYZZZZZZZZZZZZZZZZZZZZZZZZZZ"
            "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

        nanodbc::connection connection = connect();
        create_table(connection, NANODBC_TEXT("blob_test"), NANODBC_TEXT("(data BLOB)"));
        execute(
            connection, NANODBC_TEXT("insert into blob_test values ('") + s + NANODBC_TEXT("');"));

        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from blob_test;"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == s);
    }

    void catalog_list_catalogs_test()
    {
        auto conn = connect();
        REQUIRE(conn.connected());
        nanodbc::catalog catalog(conn);

        auto names = catalog.list_catalogs();
        REQUIRE(!names.empty());
    }

    void catalog_list_schemas_test()
    {
        auto conn = connect();
        REQUIRE(conn.connected());
        nanodbc::catalog catalog(conn);

        auto names = catalog.list_schemas();
        REQUIRE(!names.empty());
    }

    void catalog_columns_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);
        nanodbc::string_type const dbms = connection.dbms_name();
        REQUIRE(!dbms.empty());

        // Check we can iterate over any columns
        {
            nanodbc::catalog::columns columns = catalog.find_columns();
            long count = 0;
            while (columns.next())
            {
                // These values must not be NULL (returned as empty string)
                REQUIRE(!columns.column_name().empty());
                count++;
            }
            REQUIRE(count > 0);
        }

        // Find a table with known name and verify its known columns
        {
            nanodbc::string_type const binary_type_name = get_binary_type_name();
            REQUIRE(!binary_type_name.empty());
            nanodbc::string_type const text_type_name = get_text_type_name();
            REQUIRE(!text_type_name.empty());

            nanodbc::string_type const table_name(NANODBC_TEXT("catalog_columns_test"));
            drop_table(connection, table_name);
            execute(
                connection,
                NANODBC_TEXT("create table ") + table_name + NANODBC_TEXT("(") +
                    NANODBC_TEXT("c0 int PRIMARY KEY,") + NANODBC_TEXT("c1 smallint NOT NULL,") +
                    NANODBC_TEXT("c2 float NULL,") + NANODBC_TEXT("c3 decimal(9, 3),") +
                    NANODBC_TEXT("c4 date,") // seems more portable than datetime (SQL Server),
                                             // timestamp (PostgreSQL, MySQL)
                    +
                    NANODBC_TEXT("c5 varchar(60) DEFAULT \'sample value\',") +
                    NANODBC_TEXT("c6 varchar(120),") + NANODBC_TEXT("c7 ") + text_type_name +
                    NANODBC_TEXT(",") + NANODBC_TEXT("c8 ") + binary_type_name +
                    NANODBC_TEXT(");"));

            // Check only SQL/ODBC standard properties, skip those which are driver-specific.
            nanodbc::catalog::columns columns = catalog.find_columns(NANODBC_TEXT("%"), table_name);

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c0"));
            if (vendor_ == database_vendor::sqlite)
            {
                // NOTE: SQLite ODBC reports values inconsistent with table definition
                REQUIRE(columns.sql_data_type() == SQL_INTEGER);
                REQUIRE(columns.column_size() == 9);         // INT size is different
                REQUIRE(columns.decimal_digits() == 10);     // INT can have decimal digits
                REQUIRE(columns.nullable() == SQL_NULLABLE); // PRIMARY KEY can be NULL
                REQUIRE(columns.is_nullable() == NANODBC_TEXT("YES"));
            }
            else
            {
                if (vendor_ == database_vendor::vertica)
                {
                    REQUIRE(columns.sql_data_type() == SQL_BIGINT);
                    REQUIRE(columns.column_size() == 19);
                }
                else
                {
                    REQUIRE(columns.sql_data_type() == SQL_INTEGER);
                    REQUIRE(columns.column_size() == 10);
                }
                REQUIRE(columns.decimal_digits() == 0);
                REQUIRE(columns.nullable() == SQL_NO_NULLS);
                if (!columns.is_nullable().empty()) // nullability determined
                    REQUIRE(columns.is_nullable() == NANODBC_TEXT("NO"));
            }
            REQUIRE(
                columns.table_name() ==
                table_name); // assume common for the whole result set, check once
            REQUIRE(!columns.type_name().empty()); // data source dependant name, check once

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c1"));
            if (vendor_ == database_vendor::vertica)
            {
                REQUIRE(columns.sql_data_type() == SQL_BIGINT);
                REQUIRE(columns.column_size() == 19);
            }
            else
            {
                REQUIRE(columns.sql_data_type() == SQL_SMALLINT);
                REQUIRE(columns.column_size() == 5);
            }
            if (vendor_ == database_vendor::sqlite)
                REQUIRE(columns.decimal_digits() == 10);
            else
                REQUIRE(columns.decimal_digits() == 0);
            REQUIRE(columns.nullable() == SQL_NO_NULLS);
            if (!columns.is_nullable().empty()) // nullability determined
                REQUIRE(columns.is_nullable() == NANODBC_TEXT("NO"));

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c2"));
            REQUIRE(
                (columns.sql_data_type() == SQL_FLOAT || columns.sql_data_type() == SQL_REAL ||
                 columns.sql_data_type() == SQL_DOUBLE));
            check_data_type_size(
                NANODBC_TEXT("float"), columns.column_size(), columns.numeric_precision_radix());
            REQUIRE(columns.nullable() == SQL_NULLABLE);
            if (!columns.is_nullable().empty()) // nullability determined
                REQUIRE(columns.is_nullable() == NANODBC_TEXT("YES"));

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c3"));
            if (vendor_ == database_vendor::sqlite)
            {
#ifdef _WIN32
                REQUIRE(columns.sql_data_type() == -9); // FIXME: What is this type?
                REQUIRE(
                    columns.column_size() == 3); // FIXME: SQLite ODBC mis-reports decimal digits?
#else
                REQUIRE(columns.sql_data_type() == SQL_VARCHAR);
                REQUIRE(columns.column_size() == 9);
#endif
            }
            else
            {
                REQUIRE(
                    (columns.sql_data_type() == SQL_DECIMAL ||
                     columns.sql_data_type() == SQL_NUMERIC));
                REQUIRE(columns.column_size() == 9);
                REQUIRE(columns.decimal_digits() == 3);
            }
            REQUIRE(columns.nullable() == SQL_NULLABLE);
            if (!columns.is_nullable().empty()) // nullability determined
                REQUIRE(columns.is_nullable() == NANODBC_TEXT("YES"));

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c4"));
            if (contains_string(dbms, NANODBC_TEXT("SQLite")))
            {
                // NOTE: SQLite ODBC reports values inconsistent with table definition
                REQUIRE(columns.sql_data_type() == 91); // FIXME: What is this type?
                REQUIRE(columns.column_size() == 0);    // DATE has size Zero?
            }
            else
            {
                REQUIRE(columns.sql_data_type() == SQL_DATE);
                REQUIRE(columns.column_size() == 10); // total number of characters required to
                                                      // display the value when it is converted to
                                                      // characters
            }

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c5"));
            REQUIRE((
                columns.sql_data_type() == SQL_VARCHAR || columns.sql_data_type() == SQL_WVARCHAR));
            REQUIRE(columns.column_size() == 60);
            if (contains_string(dbms, NANODBC_TEXT("SQLite")))
                REQUIRE(columns.column_default() == NANODBC_TEXT("sample value"));
            else if (contains_string(dbms, NANODBC_TEXT("PostgreSQL")))
                REQUIRE(
                    columns.column_default() ==
                    NANODBC_TEXT("\'sample value\'::character varying"));
            else if (contains_string(dbms, NANODBC_TEXT("SQL Server")))
                REQUIRE(columns.column_default() == NANODBC_TEXT("(\'sample value\')"));
            else
                REQUIRE(columns.column_default() == NANODBC_TEXT("\'sample value\'"));

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c6"));
            REQUIRE((
                columns.sql_data_type() == SQL_VARCHAR || columns.sql_data_type() == SQL_WVARCHAR));
            REQUIRE(columns.column_size() == 120);

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c7"));
            REQUIRE(
                (columns.sql_data_type() == SQL_LONGVARCHAR ||
                 columns.sql_data_type() == SQL_WLONGVARCHAR));
            REQUIRE(
                (columns.column_size() == 2147483647 ||
                 // Vertica
                 columns.column_size() == 1048576 ||
                 // MySQL
                 columns.column_size() == 65535 ||
                 // PostgreSQL uses MaxLongVarcharSize=8190, which is configurable in odbc.ini
                 columns.column_size() == 8190 ||
                 // SQLite
                 columns.column_size() == 0));
            check_data_type_size(text_type_name, columns.column_size());

            REQUIRE(columns.next());
            REQUIRE(columns.column_name() == NANODBC_TEXT("c8"));
            REQUIRE(
                (columns.sql_data_type() == SQL_VARBINARY ||
                 columns.sql_data_type() == SQL_LONGVARBINARY || // MySQL reports SQL_LONGVARBINARY
                 columns.sql_data_type() == SQL_BINARY));        // SQLite
            // SQL Server: if n is not specified in [var]binary(n), the default length is 1
            // PostgreSQL: bytea default length is reported as 255,
            // unless ByteaAsLongVarBinary=1 option is specified in connection string.
            // Vertica: column size is 80
            if (contains_string(dbms, NANODBC_TEXT("SQLite")))
                REQUIRE(columns.column_size() == 0);
            else
                REQUIRE(columns.column_size() > 0); // no need to test exact value

            // expect no more records
            REQUIRE(!columns.next());
        }
    }

    void catalog_primary_keys_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        nanodbc::string_type const dbms = connection.dbms_name();
        REQUIRE(!dbms.empty());

        // Find a single-column primary key for table with known name
        {
            nanodbc::string_type const table_name(NANODBC_TEXT("catalog_primary_keys_simple_test"));
            drop_table(connection, table_name);
            if (contains_string(dbms, NANODBC_TEXT("SQLite")))
            {
                create_table(connection, table_name, NANODBC_TEXT("i int PRIMARY KEY"));
            }
            else
            {
                execute(
                    connection,
                    NANODBC_TEXT("create table ") + table_name +
                        NANODBC_TEXT(
                            "(i int NOT NULL, CONSTRAINT pk_simple_test PRIMARY KEY (i));"));
            }
            nanodbc::catalog::primary_keys keys = catalog.find_primary_keys(table_name);
            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("i"));
            REQUIRE(keys.column_number() == 1);
            auto const pk_simple = get_primary_key_name(NANODBC_TEXT("pk_simple_test"));
            if (!pk_simple.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_simple);
            // expect no more records
            REQUIRE(!keys.next());
        }

        // Find a multi-column primary key for table with known name
        {
            nanodbc::string_type const table_name(
                NANODBC_TEXT("catalog_primary_keys_composite_test"));
            drop_table(connection, table_name);
            execute(
                connection,
                NANODBC_TEXT("create table ") + table_name +
                    NANODBC_TEXT(
                        "(a int, b smallint, CONSTRAINT pk_composite_test PRIMARY KEY(a, b));"));

            nanodbc::catalog::primary_keys keys = catalog.find_primary_keys(table_name);
            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("a"));
            REQUIRE(keys.column_number() == 1);
            auto const pk_composite1 = get_primary_key_name(NANODBC_TEXT("pk_composite_test"));
            if (!pk_composite1.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_composite1);

            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("b"));
            REQUIRE(keys.column_number() == 2);
            auto const pk_composite2 = get_primary_key_name(NANODBC_TEXT("pk_composite_test"));
            if (!pk_composite2.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_composite2);

            // expect no more records
            REQUIRE(!keys.next());
        }
    }

    void catalog_tables_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        // Check we can iterate over any tables
        {
            nanodbc::catalog::tables tables = catalog.find_tables();
            long count = 0;
            while (tables.next())
            {
                // These two values must not be NULL (returned as empty string)
                REQUIRE(!tables.table_name().empty());
                REQUIRE(!tables.table_type().empty());
                count++;
            }
            REQUIRE(count > 0);
        }

        // Check if there are any tables (with catalog restriction)
        {
            nanodbc::string_type empty_name; // a placeholder, makes no restriction on the look-up
            nanodbc::catalog::tables tables =
                catalog.find_tables(empty_name, NANODBC_TEXT("TABLE"), empty_name, empty_name);
            long count = 0;
            while (tables.next())
            {
                // These two values must not be NULL (returned as empty string)
                REQUIRE(!tables.table_name().empty());
                REQUIRE(!tables.table_type().empty());
                count++;
            }
            REQUIRE(count > 0);
        }

        nanodbc::string_type const table_name(NANODBC_TEXT("catalog_tables_test"));

        // Find a table with known name
        {
            drop_table(connection, table_name);
            execute(
                connection, NANODBC_TEXT("create table ") + table_name + NANODBC_TEXT("(a int);"));

            // Use brute-force look-up
            {
                nanodbc::catalog::tables tables = catalog.find_tables();
                bool found = false;
                while (tables.next())
                {
                    if (table_name == tables.table_name())
                    {
                        REQUIRE(tables.table_type() == NANODBC_TEXT("TABLE"));
                        found = true;
                        break;
                    }
                }
                REQUIRE(found);
            }

            // Use SQLTables pattern search capabilities
            {
                nanodbc::catalog::tables tables = catalog.find_tables(table_name);
                // expect single record with the wanted table
                REQUIRE(tables.next());
                REQUIRE(tables.table_name() == table_name);
                REQUIRE(tables.table_type() == NANODBC_TEXT("TABLE"));
                // expect no more records
                REQUIRE(!tables.next());
            }
        }

        // Find a VIEW with known name
        {
            // Use SQLTables pattern search by name only (in any schema)
            {
                nanodbc::string_type const view_name(NANODBC_TEXT("catalog_tables_test_view"));
                try
                {
                    execute(connection, NANODBC_TEXT("DROP VIEW ") + view_name);
                }
                catch (...)
                {
                }
                execute(
                    connection,
                    NANODBC_TEXT("CREATE VIEW ") + view_name + NANODBC_TEXT(" AS SELECT a FROM ") +
                        table_name);

                nanodbc::catalog::tables tables =
                    catalog.find_tables(view_name, NANODBC_TEXT("VIEW"));
                // expect single record with the wanted table
                REQUIRE(tables.next());
                REQUIRE(tables.table_name() == view_name);
                REQUIRE(tables.table_type() == NANODBC_TEXT("VIEW"));
                // expect no more records
                REQUIRE(!tables.next());

                // Clean up, otherwise source table can not be dropped and re-created
                execute(connection, NANODBC_TEXT("DROP VIEW ") + view_name);
            }

            // Use SQLTables pattern search by name inside given schema
            // TODO: Target other databases where INFORMATION_SCHEMA support is available.
            if (connection.dbms_name().find(NANODBC_TEXT("SQL Server")) !=
                nanodbc::string_type::npos)
            {
                nanodbc::string_type const view_name(NANODBC_TEXT("TABLE_PRIVILEGES"));
                nanodbc::string_type const schema_name(NANODBC_TEXT("INFORMATION_SCHEMA"));
                nanodbc::catalog::tables tables =
                    catalog.find_tables(view_name, NANODBC_TEXT("VIEW"), schema_name);
                // expect single record with the wanted table
                REQUIRE(tables.next());
                REQUIRE(tables.table_schema() == schema_name);
                REQUIRE(tables.table_name() == view_name);
                REQUIRE(tables.table_type() == NANODBC_TEXT("VIEW"));
                // expect no more records
                REQUIRE(!tables.next());
            }
        }
    }

    void catalog_table_privileges_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        // create several tables
        create_table(
            connection, NANODBC_TEXT("catalog_table_privileges_test"), NANODBC_TEXT("i int"));

        // Check we can iterate over any tables
        {
            auto tables = catalog.find_table_privileges(NANODBC_TEXT(""));
            long count = 0;
            while (tables.next())
            {
                // These values must not be NULL (returned as empty string)
                REQUIRE(!tables.table_name().empty());
                REQUIRE(!tables.privilege().empty());
                count++;
            }
            REQUIRE(count > 0);
        }

        // Check we can find a particular table
        {
            auto tables = catalog.find_table_privileges(
                NANODBC_TEXT(""), NANODBC_TEXT("catalog_table_privileges_test"));
            long count = 0;
            std::set<nanodbc::string_type> privileges;
            while (tables.next())
            {
                // These two values must not be NULL (returned as empty string)
                REQUIRE(tables.table_name() == NANODBC_TEXT("catalog_table_privileges_test"));
                privileges.insert(tables.privilege());
                count++;
            }
            REQUIRE(count > 0);

            // verify expected privileges
            REQUIRE(!privileges.empty());
            REQUIRE(privileges.count(NANODBC_TEXT("SELECT")));
            REQUIRE(privileges.count(NANODBC_TEXT("INSERT")));
            REQUIRE(privileges.count(NANODBC_TEXT("UPDATE")));
            REQUIRE(privileges.count(NANODBC_TEXT("DELETE")));
            // there can be more
        }
    }

    void column_descriptor_test()
    {
        auto connection = connect();
        create_table(
            connection,
            NANODBC_TEXT("column_descriptor_test"),
            NANODBC_TEXT("(i int, d decimal(7,3), n numeric(7,3), f float, s varchar(60), dt date, "
                         "t timestamp)"));

        auto result =
            execute(connection, NANODBC_TEXT("select i,d,n,f,s,dt,t from column_descriptor_test;"));
        REQUIRE(result.columns() == 7);

        // i int
        REQUIRE(result.column_name(0) == NANODBC_TEXT("i"));
        REQUIRE(result.column_datatype(0) == SQL_INTEGER);
        if (vendor_ == database_vendor::sqlserver)
            REQUIRE(result.column_c_datatype(0) == SQL_C_SBIGINT);
        else if (vendor_ == database_vendor::sqlite)
            REQUIRE(result.column_c_datatype(0) == SQL_C_SBIGINT);
        REQUIRE(result.column_size(0) == 10);
        REQUIRE(result.column_decimal_digits(0) == 0);
        // d decimal(7,3)
        REQUIRE(result.column_name(1) == NANODBC_TEXT("d"));
        if (vendor_ == database_vendor::sqlite)
        {
#ifdef _WIN32
            REQUIRE(result.column_datatype(1) == -9);   // FIXME: What is this type?
            REQUIRE(result.column_c_datatype(1) == -8); // FIXME: What is this type
            REQUIRE(result.column_size(1) == 7); // FIXME: SQLite ODBC mis-reports decimal digits?
#else
            REQUIRE(result.column_datatype(1) == SQL_VARCHAR);
            REQUIRE(result.column_c_datatype(1) == SQL_C_CHAR);
            REQUIRE(result.column_size(1) == 7);
#endif
            // FIXME: SQLite ODBC mis-reports decimal digits?
            REQUIRE(result.column_decimal_digits(2) == 0);
        }
        else
        {
            REQUIRE(
                (result.column_datatype(1) == SQL_DECIMAL ||
                 result.column_datatype(1) == SQL_NUMERIC));
            REQUIRE(result.column_c_datatype(1) == SQL_C_DOUBLE);
        }
        REQUIRE(result.column_size(1) == 7);
        // n numeric(7,3)
        REQUIRE(result.column_name(2) == NANODBC_TEXT("n"));
        REQUIRE(result.column_c_datatype(2) == SQL_C_DOUBLE);
        REQUIRE(result.column_size(2) == 7);
        if (vendor_ == database_vendor::sqlite)
        {
            REQUIRE(result.column_datatype(2) == 8); // FIXME: What is this type?
            // FIXME: SQLite ODBC mis-reports decimal digits?
            REQUIRE(result.column_decimal_digits(2) == 0);
        }
        else
        {
            REQUIRE(
                (result.column_datatype(2) == SQL_DECIMAL ||
                 result.column_datatype(2) == SQL_NUMERIC));
            REQUIRE(result.column_decimal_digits(2) == 3);
        }
    }

    void date_test()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("date_test"), NANODBC_TEXT("d date"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into date_test(d) values (?);"));

            nanodbc::date d{2016, 7, 12};
            statement.bind(0, &d);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select d from date_test;"));
            REQUIRE(result.next());
            auto d = result.get<nanodbc::date>(0);
            REQUIRE(d.year == 2016);
            REQUIRE(d.month == 7);
            REQUIRE(d.day == 12);
        }
    }

    void dbms_info_test()
    {
        // A generic test to exercise the DBMS info API is callable.
        // DBMS-specific test (MySQL, SQLite, etc.) may perform extended checks.
        nanodbc::connection connection = connect();
        REQUIRE(!connection.dbms_name().empty());
        REQUIRE(!connection.dbms_version().empty());
    }

    void get_info_test()
    {
        // A generic test to exercise the DBMS info API is callable.
        // DBMS-specific test (MySQL, SQLite, etc.) may perform extended checks.
        nanodbc::connection connection = connect();
        REQUIRE(!connection.get_info<nanodbc::string_type>(SQL_DRIVER_NAME).empty());
        REQUIRE(!connection.get_info<nanodbc::string_type>(SQL_ODBC_VER).empty());

       // Test SQLUSMALLINT results
        REQUIRE(connection.get_info<unsigned short>(SQL_NON_NULLABLE_COLUMNS) == SQL_NNC_NON_NULL);

        // Test SQUINTEGER results
        REQUIRE(connection.get_info<uint32_t>(SQL_ODBC_INTERFACE_CONFORMANCE) > 0);

        // Test SQUINTEGER bitmask results
        REQUIRE((connection.get_info<uint32_t>(SQL_CREATE_TABLE) & SQL_CT_CREATE_TABLE));

        // Test SQLULEN results
        REQUIRE(connection.get_info<uint64_t>(SQL_DRIVER_HDBC) > 0);
    }

    void decimal_conversion_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::result results;
        drop_table(connection, NANODBC_TEXT("decimal_conversion_test"));
        execute(
            connection, NANODBC_TEXT("create table decimal_conversion_test (d decimal(9, 3));"));
        execute(
            connection, NANODBC_TEXT("insert into decimal_conversion_test values (12345.987);"));
        execute(connection, NANODBC_TEXT("insert into decimal_conversion_test values (5.600);"));
        execute(connection, NANODBC_TEXT("insert into decimal_conversion_test values (1.000);"));
        execute(connection, NANODBC_TEXT("insert into decimal_conversion_test values (-1.333);"));
        results = execute(
            connection, NANODBC_TEXT("select * from decimal_conversion_test order by 1 desc;"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("12345.987"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("5.600"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("1.000"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("-1.333"));
    }

    void driver_test()
    {
        auto const driver_name = connection_string_parameter(NANODBC_TEXT("DRIVER"));

        // Verify given driver, by name, is available - that is,
        // it is registered with the ODBC Driver Manager in the host environment.
        REQUIRE(!driver_name.empty());
        auto const drivers = nanodbc::list_drivers();
        bool found = std::any_of(
            drivers.cbegin(), drivers.cend(), [&driver_name](nanodbc::driver const& drv) {
                return driver_name == drv.name;
            });
        REQUIRE(found);
    }

    void exception_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::result results;

        REQUIRE_THROWS_AS(
            execute(connection, NANODBC_TEXT("THIS IS NOT VALID SQL!")), nanodbc::database_error);

        drop_table(connection, NANODBC_TEXT("exception_test"));
        execute(connection, NANODBC_TEXT("create table exception_test (i int);"));
        execute(connection, NANODBC_TEXT("insert into exception_test values (-10);"));
        execute(connection, NANODBC_TEXT("insert into exception_test values (null);"));

        results = execute(connection, NANODBC_TEXT("select * from exception_test where i = -10;"));

        REQUIRE(results.next());
        REQUIRE_THROWS_AS(results.get<nanodbc::date>(0), nanodbc::type_incompatible_error);
        REQUIRE_THROWS_AS(results.get<nanodbc::timestamp>(0), nanodbc::type_incompatible_error);

        results =
            execute(connection, NANODBC_TEXT("select * from exception_test where i is null;"));

        REQUIRE(results.next());
        REQUIRE_THROWS_AS(results.get<int>(0), nanodbc::null_access_error);
        REQUIRE_THROWS_AS(results.get<int>(42), nanodbc::index_range_error);

        nanodbc::statement statement(connection);
        REQUIRE(statement.open());
        REQUIRE(statement.connected());
        statement.close();
        REQUIRE_THROWS_AS(
            statement.prepare(NANODBC_TEXT("select * from exception_test;")),
            nanodbc::programming_error);
    }

    void execute_multiple_transaction_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::statement statement;
        nanodbc::result results;

        statement.prepare(connection, NANODBC_TEXT("select 42;"));

        {
            nanodbc::transaction transaction(connection);
            results = statement.execute();
            results.next();
            REQUIRE(results.get<int>(0) == 42);
        }

        results = statement.execute();
        results.next();
        REQUIRE(results.get<int>(0) == 42);
    }

    void execute_multiple_test()
    {
        nanodbc::connection connection = connect();
        nanodbc::statement statement(connection);
        nanodbc::prepare(statement, NANODBC_TEXT("select 42;"));

        nanodbc::result results = statement.execute();
        results.next();

        results = statement.execute();
        results.next();
        REQUIRE(results.get<int>(0) == 42);

        results = statement.execute();
        results.next();
        REQUIRE(results.get<int>(0) == 42);
    }

    template <class T>
    void integral_test_template()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("integral_test"));
        execute(
            connection,
            NANODBC_TEXT("create table integral_test (i int, f float, d double precision);"));

        nanodbc::statement statement(connection);
        prepare(statement, NANODBC_TEXT("insert into integral_test (i, f, d) values (?, ?, ?);"));

        srand(0);
        const int32_t i = rand() % 5000;
        const float f = rand() / (rand() + 1.0);
        const float d = -rand() / (rand() + 1.0);

        short p = 0;
        statement.bind(p++, &i);
        statement.bind(p++, &f);
        statement.bind(p++, &d);

        REQUIRE(statement.connected());
        execute(statement);

        nanodbc::result results = execute(connection, NANODBC_TEXT("select * from integral_test;"));
        REQUIRE(results.next());

        // NOTE: Parentheses around REQIURE() expressions are to silence error:
        //       suggest parentheses around comparison in operand of ‘==’ [-Werror=parentheses]
        T ref;
        p = 0;
        results.get_ref(p, ref);
        REQUIRE((ref == static_cast<T>(i)));
        REQUIRE((results.get<T>(p++) == Approx(static_cast<T>(i))));
        results.get_ref(p, ref);
        REQUIRE((static_cast<float>(ref) == Approx(static_cast<T>(f))));
        REQUIRE((static_cast<float>(results.get<T>(p++)) == Approx(static_cast<T>(f))));
        results.get_ref(p, ref);
        REQUIRE((static_cast<double>(ref) == Approx(static_cast<T>(d))));
        REQUIRE((static_cast<double>(results.get<T>(p++)) == Approx(static_cast<T>(d))));
    }

    template <class Fixture, class TypeList, size_t i = std::tuple_size<TypeList>::value - 1>
    struct foreach
    {
        static void run()
        {
            Fixture fixture;
            using type = typename std::tuple_element<i, TypeList>::type;
            fixture.template integral_test_template<type>();
            foreach
                <Fixture, TypeList, i - 1>::run();
        }
    };

    template <class Fixture, class TypeList>
    struct foreach<Fixture, TypeList, 0>
    {
        static void run()
        {
            Fixture fixture;
            using type = typename std::tuple_element<0, TypeList>::type;
            fixture.template integral_test_template<type>();
        }
    };

    template <class Fixture>
    void integral_test()
    {
        foreach
            <Fixture, integral_test_types>::run();
    }

    void move_test()
    {
        nanodbc::connection orig_connection = connect();
        drop_table(orig_connection, NANODBC_TEXT("move_test"));
        execute(orig_connection, NANODBC_TEXT("create table move_test (i int);"));
        execute(orig_connection, NANODBC_TEXT("insert into move_test values (10);"));

        nanodbc::connection new_connection = std::move(orig_connection);
        execute(new_connection, NANODBC_TEXT("insert into move_test values (30);"));
        execute(new_connection, NANODBC_TEXT("insert into move_test values (20);"));

        nanodbc::result orig_results =
            execute(new_connection, NANODBC_TEXT("select i from move_test order by i desc;"));
        REQUIRE(orig_results.next());
        REQUIRE(orig_results.get<int>(0) == 30);
        REQUIRE(orig_results.next());
        REQUIRE(orig_results.get<int>(0) == 20);

        nanodbc::result new_results = std::move(orig_results);
        REQUIRE(new_results.next());
        REQUIRE(new_results.get<int>(0) == 10);
    }

    void null_test()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("null_test"));
        execute(connection, NANODBC_TEXT("create table null_test (a int, b varchar(10));"));

        nanodbc::statement statement(connection);

        prepare(statement, NANODBC_TEXT("insert into null_test (a, b) values (?, ?);"));
        statement.bind_null(0);
        statement.bind_null(1);
        execute(statement);

        prepare(statement, NANODBC_TEXT("insert into null_test (a, b) values (?, ?);"));
        statement.bind_null(0, 2);
        statement.bind_null(1, 2);
        execute(statement, 2);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select a, b from null_test order by a;"));

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));

        REQUIRE(!results.next());
    }

    // checks that:         statement.bind(0, &i, 1, nullptr, nanodbc::statement::PARAM_IN);
    // works the same as:   statement.bind(0, &i, 1, nanodbc::statement::PARAM_IN);
    void nullptr_nulls_test()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("nullptr_nulls_test"));
        execute(connection, NANODBC_TEXT("create table nullptr_nulls_test (i int);"));

        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into nullptr_nulls_test (i) values (?);"));

            int i = 5;
            statement.bind(0, &i, 1, nullptr, nanodbc::statement::PARAM_IN);

            REQUIRE(statement.connected());
            execute(statement);

            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from nullptr_nulls_test;"));
            REQUIRE(results.next());

            REQUIRE(results.get<int>(0) == i);
        }

        execute(connection, NANODBC_TEXT("DELETE FROM nullptr_nulls_test;"));

        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into nullptr_nulls_test (i) values (?);"));

            int i = 5;
            statement.bind(0, &i, 1, nanodbc::statement::PARAM_IN);

            REQUIRE(statement.connected());
            execute(statement);

            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from nullptr_nulls_test;"));
            REQUIRE(results.next());

            REQUIRE(results.get<int>(0) == i);
        }
    }

    void result_iterator_test()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("result_iterator_test"));
        execute(
            connection, NANODBC_TEXT("create table result_iterator_test (i int, s varchar(10));"));
        execute(connection, NANODBC_TEXT("insert into result_iterator_test values (1, 'one');"));
        execute(connection, NANODBC_TEXT("insert into result_iterator_test values (2, 'two');"));
        execute(connection, NANODBC_TEXT("insert into result_iterator_test values (3, 'tri');"));

        // Test standard algorithm
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from result_iterator_test;"));
            REQUIRE(std::distance(begin(results), end(results)) == 3);
        }

        // Test classic for loop iteration
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from result_iterator_test;"));
            for (auto it = begin(results); it != end(results); ++it)
            {
                REQUIRE(it->get<int>(0) > 0);
                REQUIRE(it->get<nanodbc::string_type>(1).size() == 3);
            }
            REQUIRE(
                std::distance(begin(results), end(results)) ==
                0); // InputIterators only guarantee validity for single pass algorithms
        }

        // Test range-based for loop iteration
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from result_iterator_test;"));
            for (auto& row : results)
            {
                REQUIRE(row.get<int>(0) > 0);
                REQUIRE(row.get<nanodbc::string_type>(1).size() == 3);
            }
            REQUIRE(
                std::distance(begin(results), end(results)) ==
                0); // InputIterators only guarantee validity for single pass algorithms
        }
    }

    void simple_test()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        drop_table(connection, NANODBC_TEXT("simple_test"));
        execute(
            connection,
            NANODBC_TEXT("create table simple_test (sort_order int, a int, b varchar(10));"));
        execute(connection, NANODBC_TEXT("insert into simple_test values (2, 1, 'one');"));
        execute(connection, NANODBC_TEXT("insert into simple_test values (3, 2, 'two');"));
        execute(connection, NANODBC_TEXT("insert into simple_test values (4, 3, 'tri');"));
        execute(connection, NANODBC_TEXT("insert into simple_test values (1, NULL, 'z');"));

        {
            nanodbc::result results = execute(
                connection, NANODBC_TEXT("select a, b from simple_test order by sort_order;"));
            REQUIRE((bool)results);
            REQUIRE(results.rows() == 0);
            REQUIRE(results.columns() == 2);

            // From MSDN on SQLRowCount:
            // Row count is either the number of rows affected by the request
            // or -1 if the number of affected rows is not available.
            // For other statements and functions, the driver may define the value returned (...)
            // some data sources may be able to return the number of rows returned by a SELECT
            // statement.
            const bool affected_four = results.affected_rows() == 4;
            const bool affected_zero = results.affected_rows() == 0;
            const bool affected_negative_one = results.affected_rows() == -1;
            const int affected = affected_four + affected_zero + affected_negative_one;
            REQUIRE(affected != 0);
            if (!affected)
            {
                // Provide more verbose output if one of the above terms is false:
                CHECK(affected_four);
                CHECK(affected_zero);
                CHECK(affected_negative_one);
            }

            REQUIRE(results.rowset_size() == 1);
            REQUIRE(results.column_name(0) == NANODBC_TEXT("a"));
            REQUIRE(results.column_name(1) == NANODBC_TEXT("b"));

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results.next());
            // row = (null)|z
            // .....................................................................................
            REQUIRE(results.rows() == 1);
            REQUIRE(results.is_null(0));
            REQUIRE(results.is_null(NANODBC_TEXT("a")));
            REQUIRE(results.get<int>(0, -1) == -1);
            REQUIRE(results.get<int>(NANODBC_TEXT("a"), -1) == -1);
            REQUIRE(
                results.get<nanodbc::string_type>(0, NANODBC_TEXT("null")) == NANODBC_TEXT("null"));
            REQUIRE(
                results.get<nanodbc::string_type>(NANODBC_TEXT("a"), NANODBC_TEXT("null")) ==
                NANODBC_TEXT("null"));
            REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("z"));
            REQUIRE(results.get<nanodbc::string_type>(NANODBC_TEXT("b")) == NANODBC_TEXT("z"));

            int ref_int;
            results.get_ref(0, -1, ref_int);
            REQUIRE(ref_int == -1);
            results.get_ref(NANODBC_TEXT("a"), -2, ref_int);
            REQUIRE(ref_int == -2);

            nanodbc::string_type ref_str;
            results.get_ref<nanodbc::string_type>(0, NANODBC_TEXT("null"), ref_str);
            REQUIRE(ref_str == NANODBC_TEXT("null"));
            results.get_ref<nanodbc::string_type>(
                NANODBC_TEXT("a"), NANODBC_TEXT("null2"), ref_str);
            REQUIRE(ref_str == NANODBC_TEXT("null2"));

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results.next());
            // row = 1|one
            // .....................................................................................
            REQUIRE(results.get<int>(0) == 1);
            REQUIRE(results.get<int>(NANODBC_TEXT("a")) == 1);
            REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("one"));
            REQUIRE(results.get<nanodbc::string_type>(NANODBC_TEXT("b")) == NANODBC_TEXT("one"));

            nanodbc::result results_copy = results;

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results_copy.next());
            // row = 2|two
            // .....................................................................................
            REQUIRE(results_copy.get<int>(0, -1) == 2);
            REQUIRE(results_copy.get<int>(NANODBC_TEXT("a"), -1) == 2);
            REQUIRE(results_copy.get<nanodbc::string_type>(1) == NANODBC_TEXT("two"));
            REQUIRE(
                results_copy.get<nanodbc::string_type>(NANODBC_TEXT("b")) == NANODBC_TEXT("two"));

            // FIXME: not supported by the default SQL_CURSOR_FORWARD_ONLY
            // and will require SQL_ATTR_CURSOR_TYPE set to SQL_CURSOR_STATIC at least.
            // REQUIRE(results.position());

            nanodbc::result().swap(results_copy);

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results.next());
            // row = 3|tri
            // .....................................................................................
            REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("3"));
            REQUIRE(results.get<nanodbc::string_type>(NANODBC_TEXT("a")) == NANODBC_TEXT("3"));
            REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("tri"));
            REQUIRE(results.get<nanodbc::string_type>(NANODBC_TEXT("b")) == NANODBC_TEXT("tri"));

            REQUIRE(!results.next());
            REQUIRE(results.at_end());
        }

        nanodbc::connection connection_copy(connection);

        connection.disconnect();
        REQUIRE(!connection.connected());
        REQUIRE(!connection_copy.connected());
    }

    void string_test()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const nanodbc::string_type name = NANODBC_TEXT("Fred");

        drop_table(connection, NANODBC_TEXT("string_test"));
        execute(connection, NANODBC_TEXT("create table string_test (s varchar(10));"));

        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into string_test(s) values(?)"));
        query.bind(0, name.c_str());
        nanodbc::execute(query);

        nanodbc::result results = execute(connection, NANODBC_TEXT("select s from string_test;"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("Fred"));

        nanodbc::string_type ref;
        results.get_ref(0, ref);
        REQUIRE(ref == name);
    }
    void string_vector_test()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const std::vector<nanodbc::string_type> first_name = {NANODBC_TEXT("Fred"), NANODBC_TEXT("Barney"), NANODBC_TEXT("Dino")};
        const std::vector<nanodbc::string_type> last_name = {NANODBC_TEXT("Flintstone"), NANODBC_TEXT("Rubble"), NANODBC_TEXT("")};
        const std::vector<nanodbc::string_type> gender = {NANODBC_TEXT("Male"), NANODBC_TEXT("Male"), NANODBC_TEXT("")};

        drop_table(connection, NANODBC_TEXT("string_vector_test"));
        execute(connection, NANODBC_TEXT("create table string_vector_test (first varchar(10), last varchar(10), gender varchar(10));"));

        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into string_vector_test(first, last, gender) values(?, ?, ?)"));

        // Without nulls
        query.bind_strings(0, first_name);

        // With null vector
        bool nulls[3] = {false, false, true};
        query.bind_strings(1, last_name, nulls);

        // With null sentry
        query.bind_strings(2, gender, NANODBC_TEXT(""));

        nanodbc::execute(query, 3);

        nanodbc::result results = execute(connection, NANODBC_TEXT("select first,last,gender from string_vector_test"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("Fred"));
        REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("Flintstone"));
        REQUIRE(results.get<nanodbc::string_type>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("Barney"));
        REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("Rubble"));
        REQUIRE(results.get<nanodbc::string_type>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string_type>(0) == NANODBC_TEXT("Dino"));
        REQUIRE(results.is_null(1));
        REQUIRE(results.is_null(2));
    }

    void time_test()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("time_test"), NANODBC_TEXT("t time"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into time_test(t) values (?);"));

            nanodbc::time t{11, 45, 59};
            statement.bind(0, &t);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select t from time_test;"));
            REQUIRE(result.next());
            auto t = result.get<nanodbc::time>(0);
            REQUIRE(t.hour == 11);
            REQUIRE(t.min == 45);
            REQUIRE(t.sec == 59);
        }
    }

    void transaction_test()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("transaction_test"));
        if (vendor_ == database_vendor::mysql)
            execute(
                connection, NANODBC_TEXT("create table transaction_test (i int) ENGINE = INNODB;"));
        else
            execute(connection, NANODBC_TEXT("create table transaction_test (i int);"));

        nanodbc::statement statement(connection);
        prepare(statement, NANODBC_TEXT("insert into transaction_test (i) values (?);"));

        static const int elements = 10;
        int data[elements] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        statement.bind(0, data, elements);
        execute(statement, elements);

        static const nanodbc::string_type::value_type* query =
            NANODBC_TEXT("select count(1) from transaction_test;");

        check_rows_equal(execute(connection, query), 10);

        REQUIRE(connection.transactions() == 0);
        {
            nanodbc::transaction transaction(connection);
            REQUIRE(connection.transactions() == 1);
            execute(connection, NANODBC_TEXT("delete from transaction_test;"));
            check_rows_equal(execute(connection, query), 0);
            REQUIRE(connection.transactions() == 1);
            // ~transaction() calls rollback()
        }
        REQUIRE(connection.transactions() == 0);

        check_rows_equal(execute(connection, query), 10);

        REQUIRE(connection.transactions() == 0);
        {
            nanodbc::transaction transaction(connection);
            REQUIRE(connection.transactions() == 1);
            execute(connection, NANODBC_TEXT("delete from transaction_test;"));
            check_rows_equal(execute(connection, query), 0);
            REQUIRE(connection.transactions() == 1);
            transaction.rollback(); // only requests rollback performed in ~transaction()
            REQUIRE(connection.transactions() == 1); // transaction not released yet
        }
        REQUIRE(connection.transactions() == 0);

        check_rows_equal(execute(connection, query), 10);

        REQUIRE(connection.transactions() == 0);
        {
            nanodbc::transaction transaction(connection);
            REQUIRE(connection.transactions() == 1);
            execute(connection, NANODBC_TEXT("delete from transaction_test;"));
            check_rows_equal(execute(connection, query), 0);
            REQUIRE(connection.transactions() == 1);
            transaction.commit(); // performs actual commit and releases transaction
            REQUIRE(connection.transactions() == 0);
        }
        REQUIRE(connection.transactions() == 0);

        check_rows_equal(execute(connection, query), 0);
    }

    void while_not_end_iteration_test()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("while_not_end_iteration_test"));
        execute(connection, NANODBC_TEXT("create table while_not_end_iteration_test (i int);"));
        execute(connection, NANODBC_TEXT("insert into while_not_end_iteration_test values (1);"));
        execute(connection, NANODBC_TEXT("insert into while_not_end_iteration_test values (2);"));
        execute(connection, NANODBC_TEXT("insert into while_not_end_iteration_test values (3);"));
        nanodbc::result results = execute(
            connection,
            NANODBC_TEXT("select * from while_not_end_iteration_test order by 1 desc;"));
        int i = 3;
        while (!results.at_end())
        {
            results.next();
            REQUIRE(results.get<int>(0) == i--);
        }
    }

    void while_next_iteration_test()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("while_next_iteration_test"));
        execute(connection, NANODBC_TEXT("create table while_next_iteration_test (i int);"));
        execute(connection, NANODBC_TEXT("insert into while_next_iteration_test values (1);"));
        execute(connection, NANODBC_TEXT("insert into while_next_iteration_test values (2);"));
        execute(connection, NANODBC_TEXT("insert into while_next_iteration_test values (3);"));
        nanodbc::result results = execute(
            connection, NANODBC_TEXT("select * from while_next_iteration_test order by 1 desc;"));
        int i = 3;
        while (results.next())
        {
            REQUIRE(results.get<int>(0) == i--);
        }
    }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
