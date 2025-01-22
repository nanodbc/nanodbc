#ifndef NANODBC_TEST_CASE_FIXTURE_H
#define NANODBC_TEST_CASE_FIXTURE_H

#include "base_test_fixture.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <set>
#include <tuple>
#include <vector>

#ifdef _MSC_VER
#include <comutil.h> // static nanodbc links w/ #pragma comment(lib, "comsuppw.lib")
#include <nanodbc/variant_row_cached_result.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // conversion from 'T1' to 'T2' possible loss of data
#endif

struct test_case_fixture : public base_test_fixture
{
    // To invoke a unit test over all integral types, use:
    //
    typedef std::tuple<
        short,
        unsigned short,
        int,
        unsigned int,
        int32_t,
        uint32_t,
        long int,
        unsigned long int,
        int64_t,
        uint64_t,
        signed long long,
        unsigned long long,
        float,
        double>
        integral_test_types;

    // Test Cases

    void test_batch_insert_integral()
    {
        auto conn = connect();
        create_table(conn, NANODBC_TEXT("test_batch_insert_integer"), NANODBC_TEXT("(i int)"));

        std::size_t const batch_size = 9;
        int values[batch_size] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        nanodbc::statement stmt(conn);
        prepare(stmt, NANODBC_TEXT("insert into test_batch_insert_integer(i) values (?)"));
        REQUIRE(stmt.parameters() == 1);

        stmt.bind(0, values, batch_size);

        nanodbc::transact(stmt, batch_size);
        {
            auto result = nanodbc::execute(
                conn, NANODBC_TEXT("select i from test_batch_insert_integer order by i asc"));
            std::size_t i = 0;
            while (result.next())
            {
                REQUIRE(result.get<int>(0) == values[i]);
                ++i;
            }
            REQUIRE(i == batch_size);
        }
    }
#ifdef NANODBC_HAS_STD_OPTIONAL
    void test_batch_insert_integral_optional()
    {
        auto conn = connect();
        create_table(
            conn, NANODBC_TEXT("test_batch_insert_integral_optional"), NANODBC_TEXT("(i int)"));

        std::size_t const batch_size = 9;
        int values[batch_size] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        nanodbc::statement stmt(conn);
        prepare(
            stmt, NANODBC_TEXT("insert into test_batch_insert_integral_optional(i) values (?)"));
        REQUIRE(stmt.parameters() == 1);

        stmt.bind(0, values, batch_size);

        nanodbc::transact(stmt, batch_size);
        {
            auto result = nanodbc::execute(
                conn,
                NANODBC_TEXT("select i from test_batch_insert_integral_optional order by i asc"));
            std::size_t i = 0;
            while (result.next())
            {
                REQUIRE(*result.get<std::optional<int>>(0) == values[i]);
                ++i;
            }
            REQUIRE(i == batch_size);
        }
    }
#endif
    void test_batch_insert_mixed()
    {
        std::size_t const batch_size = 9;
        int integers[batch_size] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        float floats[batch_size] = {1.123f, 2.345f, 3.1f, 4.5f, 5.678f, 6.f, 7.89f, 8.90f, 9.1234f};
        nanodbc::string::value_type strings[batch_size][60] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring"),
            NANODBC_TEXT(""),
            NANODBC_TEXT("sixth string"),
            NANODBC_TEXT("A"),
            NANODBC_TEXT("ninth string")};

        // Test binding strings as variable-length parameter at different positions
        auto conn = connect();
        for (auto strings_param_pos : {2, 1, 0})
        {
            create_table(
                conn,
                NANODBC_TEXT("test_batch_insert_mixed"),
                NANODBC_TEXT("(i int, s varchar(60), f float)"));

            nanodbc::string insert(NANODBC_TEXT("insert into test_batch_insert_mixed "));
            if (strings_param_pos == 2)
                insert += NANODBC_TEXT("(i, f, s)");
            else if (strings_param_pos == 1)
                insert += NANODBC_TEXT("(i, s, f)");
            else if (strings_param_pos == 0)
                insert += NANODBC_TEXT("(s, i, f)");
            insert += NANODBC_TEXT(" values(?, ?, ?)");

            nanodbc::statement stmt(conn);
            prepare(stmt, insert);
            if (strings_param_pos == 2)
            {
                stmt.bind(0, integers, batch_size);
                stmt.bind(1, floats, batch_size);
                stmt.bind_strings(2, strings);
            }
            else if (strings_param_pos == 1)
            {
                stmt.bind(0, integers, batch_size);
                stmt.bind_strings(1, strings);
                stmt.bind(2, floats, batch_size);
            }
            else if (strings_param_pos == 0)
            {
                stmt.bind_strings(0, strings);
                stmt.bind(1, integers, batch_size);
                stmt.bind(2, floats, batch_size);
            }

            nanodbc::transact(stmt, batch_size);
            {
                auto result = nanodbc::execute(
                    conn,
                    NANODBC_TEXT("select i, f, s from test_batch_insert_mixed order by i asc"));
                std::size_t i = 0;
                while (result.next())
                {
                    REQUIRE(result.get<int>(0) == integers[i]);
                    REQUIRE(
                        result.get<float>(1) ==
                        floats[i]); // exact test might fail, switch to Approx
                    REQUIRE(result.get<nanodbc::string>(2) == strings[i]);
                    ++i;
                }
                REQUIRE(i == batch_size);
            }
        }
    }

    template <std::size_t BatchSize, std::size_t MaxValueSize>
    void test_batch_insert_string_template(
        nanodbc::connection& conn,
        nanodbc::string::value_type const (&strings)[BatchSize][MaxValueSize])
    {
        create_table(
            conn, NANODBC_TEXT("test_batch_insert_string"), NANODBC_TEXT("(s varchar(60))"));

        nanodbc::statement stmt(conn);
        prepare(stmt, NANODBC_TEXT("insert into test_batch_insert_string(s) values (?)"));
        REQUIRE(stmt.parameters() == 1);
        stmt.bind_strings(0, strings);

        nanodbc::transact(stmt, BatchSize);
        {
            auto result =
                nanodbc::execute(conn, NANODBC_TEXT("select s from test_batch_insert_string"));
            std::size_t i = 0;
            while (result.next())
            {
                REQUIRE(result.get<nanodbc::string>(0) == strings[i]);
                ++i;
            }
            REQUIRE(i == BatchSize);
        }
    }

    void test_batch_insert_string()
    {
        auto conn = connect();

        // Test input buffer lengths smaller than and equal to column size (varchar(60)).
        std::size_t const batch_size = 5;
        nanodbc::string::value_type strings25[batch_size][25] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings25);

        nanodbc::string::value_type strings27[batch_size][27] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings27);

        nanodbc::string::value_type strings30[batch_size][30] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings30);

        nanodbc::string::value_type strings41[batch_size][41] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings41);

        nanodbc::string::value_type strings55[batch_size][55] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings55);

        nanodbc::string::value_type strings60[batch_size][60] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring")};
        test_batch_insert_string_template(conn, strings60);
    }

    void test_blob()
    {
        nanodbc::string s = NANODBC_TEXT(
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
        create_table(connection, NANODBC_TEXT("test_blob"), NANODBC_TEXT("(data BLOB)"));
        execute(
            connection, NANODBC_TEXT("insert into test_blob values ('") + s + NANODBC_TEXT("');"));

        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob;"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == s);
    }

    void test_catalog_columns()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);
        nanodbc::string const dbms = connection.dbms_name();
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
            nanodbc::string const binary_type_name = get_binary_type_name();
            REQUIRE(!binary_type_name.empty());
            nanodbc::string const text_type_name = get_text_type_name();
            REQUIRE(!text_type_name.empty());

            nanodbc::string const table_name(NANODBC_TEXT("test_catalog_columns"));
            drop_table(connection, table_name);
            execute(
                connection,
                NANODBC_TEXT("create table ") + table_name + NANODBC_TEXT("(") +
                    NANODBC_TEXT("c0 int PRIMARY KEY,") + NANODBC_TEXT("c1 smallint NOT NULL,") +
                    NANODBC_TEXT("c2 float NULL,") + NANODBC_TEXT("c3 decimal(9, 3),") +
                    NANODBC_TEXT("c4 date,") // seems more portable than datetime (SQL Server),
                                             // timestamp (PostgreSQL, MySQL)
                    + NANODBC_TEXT("c5 varchar(60) DEFAULT \'sample value\',") +
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
            // FIXME: SQLite ODBC mis-reports decimal digits? Causing columns.column_size() == 3.
            if (vendor_ == database_vendor::sqlite)
            {
#if defined _WIN32
                REQUIRE(
                    columns.sql_data_type() ==
                    -9); // FIXME: If not SQL_WVARCHAR(-9), what is this type?
                REQUIRE(columns.column_size() == 3);
#elif defined __APPLE__
                REQUIRE(columns.sql_data_type() == SQL_VARCHAR);
                REQUIRE(columns.column_size() == 3);
#else
                REQUIRE(columns.sql_data_type() == SQL_VARCHAR);
                REQUIRE(columns.column_size() == 3);
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
                REQUIRE(columns.sql_data_type() == SQL_TYPE_DATE);
                REQUIRE(columns.column_size() == 0); // DATE has size Zero?
            }
            else
            {
                REQUIRE(columns.sql_data_type() == SQL_DATE);
                if (contains_string(dbms, NANODBC_TEXT("MySQL")))
                {
                    // MySQL Connector 8.x will reports for COLUMNS_SIZE field of SQLColumns
                    // resultset Apparently, MySQL driver can report NULL for COLUMN_SIZE
                    // https://dev.mysql.com/doc/relnotes/connector-odbc/en/news-8-0-33.html
                }
                else
                {
                    // total number of characters required to display the value
                    // when it is converted to characters
                    REQUIRE(columns.column_size() == 10);
                }
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
            // unless ByteaAsLongVarBinary=1 (default) option is specified in connection string.
            // See https://github.com/nanodbc/nanodbc/issues/131
            // Vertica: column size is 80
            if (contains_string(dbms, NANODBC_TEXT("SQLite")))
                REQUIRE(columns.column_size() == 0);
            else
                REQUIRE(
                    (columns.column_size() > 0 ||
                     columns.column_size() == SQL_NO_TOTAL)); // no need to test exact value

            // expect no more records
            REQUIRE(!columns.next());
        }
    }

    void test_catalog_list_catalogs()
    {
        auto conn = connect();
        REQUIRE(conn.connected());
        nanodbc::catalog catalog(conn);

        auto names = catalog.list_catalogs();
        REQUIRE(!names.empty());
    }

    void test_catalog_list_schemas()
    {
        auto conn = connect();
        REQUIRE(conn.connected());
        nanodbc::catalog catalog(conn);

        auto names = catalog.list_schemas();
        REQUIRE(!names.empty());
    }

    void test_catalog_list_table_types()
    {
        auto conn = connect();
        REQUIRE(conn.connected());
        nanodbc::catalog catalog(conn);

        auto types = catalog.list_table_types();
        REQUIRE(!types.empty());
    }

    void test_catalog_primary_keys()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        nanodbc::string const dbms = connection.dbms_name();
        REQUIRE(!dbms.empty());

        // Find a single-column primary key for table with known name
        {
            nanodbc::string const table_name(NANODBC_TEXT("test_catalog_primary_keys_simple"));
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
                            "(i int NOT NULL, CONSTRAINT test_pk_simple PRIMARY KEY (i));"));
            }
            nanodbc::catalog::primary_keys keys = catalog.find_primary_keys(table_name);
            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("i"));
            REQUIRE(keys.column_number() == 1);
            auto const pk_simple = get_primary_key_name(NANODBC_TEXT("test_pk_simple"));
            if (!pk_simple.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_simple);
            // expect no more records
            REQUIRE(!keys.next());
        }

        // Find a multi-column primary key for table with known name
        {
            nanodbc::string const table_name(NANODBC_TEXT("test_catalog_primary_keys_composite"));
            drop_table(connection, table_name);
            execute(
                connection,
                NANODBC_TEXT("create table ") + table_name +
                    NANODBC_TEXT(
                        "(a int, b smallint, CONSTRAINT test_pk_composite PRIMARY KEY(a, b));"));

            nanodbc::catalog::primary_keys keys = catalog.find_primary_keys(table_name);
            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("a"));
            REQUIRE(keys.column_number() == 1);
            auto const pk_composite1 = get_primary_key_name(NANODBC_TEXT("test_pk_composite"));
            if (!pk_composite1.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_composite1);

            REQUIRE(keys.next());
            REQUIRE(keys.table_name() == table_name);
            REQUIRE(keys.column_name() == NANODBC_TEXT("b"));
            REQUIRE(keys.column_number() == 2);
            auto const pk_composite2 = get_primary_key_name(NANODBC_TEXT("test_pk_composite"));
            if (!pk_composite2.empty()) // constraint relevant
                REQUIRE(keys.primary_key_name() == pk_composite2);

            // expect no more records
            REQUIRE(!keys.next());
        }
    }

    void test_catalog_procedure_columns()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        // Check we can iterate over any procedures
        {
            nanodbc::catalog::procedures procedures = catalog.find_procedures();
            long count = 0;
            while (procedures.next())
            {
                // This values (procedure name) must not be NULL (returned as empty string)
                REQUIRE(!procedures.procedure_name().empty());
                count++;
            }
            REQUIRE(count > 0);
        }

        nanodbc::string const procedure_name(NANODBC_TEXT("test_catalog_procedure"));

        // Find a procedure with known name
        {
            try
            {
                nanodbc::result results =
                    execute(connection, NANODBC_TEXT("DROP PROCEDURE " + procedure_name));
            }
            catch (...)
            {
            }
            execute(
                connection,
                NANODBC_TEXT("CREATE PROCEDURE " + procedure_name) +
                    NANODBC_TEXT(" @arg_varchar VARCHAR(10), @arg_int INT "
                                 "AS "
                                 "BEGIN "
                                 "        SELECT @arg_varchar AS A, @arg_int AS B, GETDATE() AS C "
                                 "END;"));

            // Use brute-force look-up
            {
                nanodbc::catalog::procedures procedures = catalog.find_procedures();
                bool found = false;
                while (procedures.next())
                {
                    // Can not expect name to be exactly the same.
                    if (procedures.procedure_name().find(procedure_name) != std::string::npos)
                    {
                        // Unclear whether we can form reliable expectations around the procedure
                        // type
                        found = true;
                        break;
                    }
                }
                REQUIRE(found);
            }

            // Use SQLProcedures pattern search capabilities
            {
                nanodbc::catalog::procedures procedures = catalog.find_procedures(procedure_name);
                // expect single record with the wanted procedure
                REQUIRE(procedures.next());
                REQUIRE(procedures.procedure_name().find(procedure_name) != std::string::npos);
                // expect no more records
                REQUIRE(!procedures.next());
            }
        }
        // Now over to find_procedure_columns
        {
            // Check we can iterate over any columns
            {
                nanodbc::catalog::procedure_columns columns = catalog.find_procedure_columns();
                long count = 0;
                while (columns.next())
                {
                    // These values must not be NULL (returned as empty string)
                    REQUIRE(!columns.column_name().empty());
                    count++;
                }
                REQUIRE(count > 0);
            }
            // Find a procedure with known name and verify its known columns
            {
                nanodbc::catalog::procedure_columns columns =
                    catalog.find_procedure_columns(NANODBC_TEXT("%"), procedure_name);
                long count = 0;
                while (columns.next())
                {
                    // Verify that the expected columns make an appearance
                    // as well as column_type is as expected.
                    // All of the remaining attribtues are shared with the SQLColumns
                    // API call and are tested there
                    if (columns.column_name().find(NANODBC_TEXT("arg_int")) != std::string::npos ||
                        columns.column_name().find(NANODBC_TEXT("arg_varchar")) !=
                            std::string::npos)
                    {
                        REQUIRE(columns.column_type() == SQL_PARAM_INPUT);
                        count++;
                    }
                }
                REQUIRE(count == 2);
            }
        }
    }

    void test_catalog_tables()
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
            nanodbc::string empty_name; // a placeholder, makes no restriction on the look-up
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

        nanodbc::string const table_name(NANODBC_TEXT("test_catalog_tables"));

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
                nanodbc::string const view_name(NANODBC_TEXT("test_catalog_tables_view"));
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
            if (connection.dbms_name().find(NANODBC_TEXT("SQL Server")) != nanodbc::string::npos)
            {
                nanodbc::string const view_name(NANODBC_TEXT("TABLE_PRIVILEGES"));
                nanodbc::string const schema_name(NANODBC_TEXT("INFORMATION_SCHEMA"));
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

    void test_catalog_table_privileges()
    {
        nanodbc::connection connection = connect();
        nanodbc::catalog catalog(connection);

        // create several tables
        create_table(
            connection, NANODBC_TEXT("test_catalog_table_privileges"), NANODBC_TEXT("i int"));

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
                NANODBC_TEXT(""), NANODBC_TEXT("test_catalog_table_privileges"));
            long count = 0;
            std::set<nanodbc::string> privileges;
            while (tables.next())
            {
                // These two values must not be NULL (returned as empty string)
                REQUIRE(tables.table_name() == NANODBC_TEXT("test_catalog_table_privileges"));
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

    void test_column_descriptor()
    {
        auto connection = connect();
        create_table(
            connection,
            NANODBC_TEXT("test_column_descriptor"),
            NANODBC_TEXT("(i int, d decimal(7,3), n numeric(7,3), f float, s varchar(60), dt date, "
                         "t timestamp)"));

        auto result =
            execute(connection, NANODBC_TEXT("select i,d,n,f,s,dt,t from test_column_descriptor;"));
        REQUIRE(result.columns() == 7);

        // i int
        REQUIRE(result.column_name(0) == NANODBC_TEXT("i"));
        REQUIRE(result.column_datatype(0) == SQL_INTEGER);
        if (vendor_ == database_vendor::sqlserver)
        {
            REQUIRE(result.column_c_datatype(0) == SQL_C_SBIGINT);
        }
        else if (vendor_ == database_vendor::sqlite)
        {
            std::string const type_name = nanodbc::test::convert(result.column_datatype_name(0));
            REQUIRE_THAT(type_name, Catch::Contains("int", Catch::CaseSensitive::No));
            REQUIRE(result.column_c_datatype(0) == SQL_C_SBIGINT);
        }
        REQUIRE(result.column_size(0) == 10);
        REQUIRE(result.column_decimal_digits(0) == 0);
        // d decimal(7,3)
        REQUIRE(result.column_name(1) == NANODBC_TEXT("d"));
        if (vendor_ == database_vendor::sqlite)
        {
#ifdef _WIN32
            REQUIRE(result.column_datatype_name(1) == NANODBC_TEXT("decimal"));
            REQUIRE(
                result.column_datatype(1) ==
                -9); // FIXME: If not SQL_WVARCHAR(-9), what is this type?
            REQUIRE(result.column_c_datatype(1) == -8); // FIXME: What is this type?
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
            REQUIRE(result.column_c_datatype(1) == SQL_C_CHAR);
        }
        REQUIRE(result.column_size(1) == 7);
        // n numeric(7,3)
        REQUIRE(result.column_name(2) == NANODBC_TEXT("n"));
        REQUIRE(result.column_size(2) == 7);
        if (vendor_ == database_vendor::sqlite)
        {
            REQUIRE(result.column_datatype_name(2) == NANODBC_TEXT("numeric"));
            REQUIRE(result.column_datatype(2) == 8); // FIXME: What is this type?
            // FIXME: SQLite ODBC mis-reports decimal digits?
            REQUIRE(result.column_decimal_digits(2) == 0);
            REQUIRE(result.column_c_datatype(2) == SQL_C_DOUBLE);
        }
        else
        {
            REQUIRE(
                (result.column_datatype(2) == SQL_DECIMAL ||
                 result.column_datatype(2) == SQL_NUMERIC));
            REQUIRE(result.column_decimal_digits(2) == 3);
            REQUIRE(result.column_c_datatype(2) == SQL_C_CHAR);
        }
    }

    void test_connection_environment()
    {
        // default constructor
        {
            nanodbc::connection c;
            REQUIRE(c.native_env_handle() == nullptr);
            REQUIRE(c.native_dbc_handle() == nullptr);
        }

        // construct with connection string
        {
            nanodbc::connection c{connection_string_};
            REQUIRE(c.native_env_handle() != nullptr);
            REQUIRE(c.native_dbc_handle() != nullptr);
        }

        // on-demand environment allocation/deallocation
        {
            nanodbc::connection c;
            c.allocate();
            REQUIRE(c.native_env_handle() != nullptr);
            REQUIRE(c.native_dbc_handle() != nullptr);
            c.deallocate();
            REQUIRE(c.native_env_handle() == nullptr);
            REQUIRE(c.native_dbc_handle() == nullptr);
        }
    }

    void test_date()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("test_date"), NANODBC_TEXT("d date"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_date(d) values (?);"));
            REQUIRE(statement.parameters() == 1);

            nanodbc::date d{2016, 7, 12};
            statement.bind(0, &d);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select d from test_date;"));

            REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
#if (ODBCVER > SQL_OV_ODBC2)
            REQUIRE(result.column_datatype(0) == SQL_TYPE_DATE);
#else
            REQUIRE(result.column_datatype(0) == SQL_DATE);
#endif
            REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("date"));

            REQUIRE(result.next());
            auto d = result.get<nanodbc::date>(0);
            REQUIRE(d.year == 2016);
            REQUIRE(d.month == 7);
            REQUIRE(d.day == 12);
        }
    }

#ifdef NANODBC_HAS_STD_OPTIONAL
    void test_date_optional()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("test_date_optional"), NANODBC_TEXT("d date"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_date_optional(d) values (?);"));
            REQUIRE(statement.parameters() == 1);

            nanodbc::date d{2016, 7, 12};
            statement.bind(0, &d);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select d from test_date_optional;"));

            REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
#if (ODBCVER > SQL_OV_ODBC2)
            REQUIRE(result.column_datatype(0) == SQL_TYPE_DATE);
#else
            REQUIRE(result.column_datatype(0) == SQL_DATE);
#endif
            REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("date"));

            REQUIRE(result.next());
            auto d = result.get<std::optional<nanodbc::date>>(0);
            REQUIRE((*d).year == 2016);
            REQUIRE((*d).month == 7);
            REQUIRE((*d).day == 12);
        }
    }
#endif
    void test_dbms_info()
    {
        // A generic test to exercise the DBMS info API is callable.
        // DBMS-specific test (MySQL, SQLite, etc.) may perform extended checks.
        nanodbc::connection connection = connect();
        REQUIRE(!connection.dbms_name().empty());
        REQUIRE(!connection.dbms_version().empty());
    }

    void test_decimal_conversion()
    {
        nanodbc::connection connection = connect();
        nanodbc::result results;
        drop_table(connection, NANODBC_TEXT("test_decimal_conversion"));
        execute(
            connection, NANODBC_TEXT("create table test_decimal_conversion (d decimal(9, 3));"));
        execute(
            connection, NANODBC_TEXT("insert into test_decimal_conversion values (12345.987);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (5.600);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (1.000);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (-1.333);"));
        results = execute(
            connection, NANODBC_TEXT("select * from test_decimal_conversion order by 1 desc;"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("12345.987"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("5.600"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("1.000"));

        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("-1.333"));
    }

    template <class T>
    void test_decimal_to_integral_conversion_template()
    {
        nanodbc::connection connection = connect();
        nanodbc::result results;
        drop_table(connection, NANODBC_TEXT("test_decimal_conversion"));
        execute(
            connection, NANODBC_TEXT("create table test_decimal_conversion (d decimal(9, 3));"));
        execute(
            connection, NANODBC_TEXT("insert into test_decimal_conversion values (12345.987);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (5.600);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (1.000);"));
        results = execute(
            connection, NANODBC_TEXT("select * from test_decimal_conversion order by 1 desc;"));

        REQUIRE(results.next());
        REQUIRE(results.get<T>(0) == static_cast<T>(12345.987));
        REQUIRE(results.next());
        REQUIRE(results.get<T>(0) == static_cast<T>(5.6));
        REQUIRE(results.next());
        REQUIRE(results.get<T>(0) == static_cast<T>(1.0));
    }

    void test_driver()
    {
        auto const driver_name = connection_string_parameter(NANODBC_TEXT("DRIVER"));

        // Verify given driver, by name, is available - that is,
        // it is registered with the ODBC Driver Manager in the host environment.
        REQUIRE(!driver_name.empty());
        auto const drivers = nanodbc::list_drivers();
        bool found = std::any_of(
            drivers.cbegin(),
            drivers.cend(),
            [&driver_name](nanodbc::driver const& drv) { return driver_name == drv.name; });
        REQUIRE(found);
    }

    void test_driver_info()
    {
        // A generic test to exercise the ODBC driver info API is callable.
        // Driver-specific tests may perform extended checks.
        nanodbc::connection connection = connect();
        REQUIRE(!connection.driver_name().empty());
        REQUIRE(!connection.driver_version().empty());
    }

    void test_datasources()
    {
        auto const dsns = nanodbc::list_datasources();
        bool test_dsn_found = std::any_of(
            dsns.cbegin(),
            dsns.cend(),
            [](nanodbc::datasource const& dsn)
            { return dsn.name == nanodbc::test::convert("testdsn"); });
        if (test_dsn_found)
        {
            auto const driver_name = connection_string_parameter(NANODBC_TEXT("DRIVER"));
            REQUIRE(!driver_name.empty());

            bool found = std::any_of(
                dsns.cbegin(),
                dsns.cend(),
                [&driver_name](nanodbc::datasource const& dsn) {
                    return dsn.name == nanodbc::test::convert("testdsn") &&
                           dsn.driver == driver_name;
                });
            REQUIRE(found);
        }
    }

    void test_error()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("test_error"));
        nanodbc::string create_table_sql = NANODBC_TEXT("create table test_error (i int not null");
        if (vendor_ == database_vendor::mysql)
        {
            create_table_sql =
                create_table_sql + NANODBC_TEXT(", constraint test_error_pk primary key (i)");
        }
        else
        {
            create_table_sql =
                create_table_sql + NANODBC_TEXT(" constraint test_error_pk primary key");
        }
        if (vendor_ == database_vendor::vertica)
        {
            create_table_sql = create_table_sql + NANODBC_TEXT(" enabled");
        }
        create_table_sql = create_table_sql + NANODBC_TEXT(");");

        execute(connection, create_table_sql);

        nanodbc::statement statement(connection);
        prepare(statement, NANODBC_TEXT("insert into test_error (i) values (0);"));
        execute(statement);

        nanodbc::database_error error = nanodbc::database_error(nullptr, 0);

        try
        {
            statement.execute();
        }
        catch (const nanodbc::database_error& e)
        {
            error = e;
        }

        struct error_result_t
        {
            int n = 0;
            std::string s;
            std::string w;
            error_result_t() = default;
            error_result_t(int n, std::string s, std::string w)
                : n(n)
                , s(s)
                , w(w)
            {
            }
        } error_result;

        switch (vendor_)
        {
        case database_vendor::mysql:
            error_result = {1062, "23000", "Duplicate entry"};
            break;
        case database_vendor::oracle:
            error_result = {1, "25S03", "ORA-00001"};
            break;
        case database_vendor::postgresql:
            error_result = {7, "23505", "duplicate key value violates unique constraint"};
            break;
        case database_vendor::sqlite:
            // Skip checking SQL Native Code as some versions of SQLite3 ODBC Driver
            // report 19 while other report 0.
            error_result = {-1, "HY000", "UNIQUE constraint"};
            break;
        case database_vendor::sqlserver:
            // 01000: [Microsoft][ODBC Driver 17 for SQL Server][SQL Server]Violation of PRIMARY KEY
            // constraint 'test_error_pk'. [Microsoft][ODBC Driver 17 for SQL Server][SQL Server]The
            // statement has been terminated.
            error_result = {3621, "01000", "Violation of PRIMARY KEY constraint"};
            break;
        case database_vendor::vertica:
            // TODO: Validate vertica
            //  https://www.vertica.com/docs/11.1.x/HTML/Content/Authoring/ErrorCodes/SqlState-23505.htm
            error_result = {6745, "23505", "Duplicate key values"};
            break;
        default:
            FAIL("Database vendor is unknown.");
        }

        // Negative means skip
        // TODO: It seems later versions or version on Linux of
        // - PostgreSQL ODBC driver changed the code from 7 to 1
        // - SQL Server driver changed the code from 3621 to 2627 and state from 01000 to 23000
        // if (error_result.n >= 0)
        //     REQUIRE(error.native() == error_result.n);
        // REQUIRE_THAT(error.state(), Catch::Matches(error_result.s));
        REQUIRE_THAT(error.what(), Catch::Contains(error_result.w));
    }

    void test_exception()
    {
        nanodbc::connection connection = connect();
        nanodbc::result results;

        REQUIRE_THROWS_AS(
            execute(connection, NANODBC_TEXT("THIS IS NOT VALID SQL!")), nanodbc::database_error);

        drop_table(connection, NANODBC_TEXT("test_exception"));
        execute(connection, NANODBC_TEXT("create table test_exception (i int);"));
        execute(connection, NANODBC_TEXT("insert into test_exception values (-10);"));
        execute(connection, NANODBC_TEXT("insert into test_exception values (null);"));

        results = execute(connection, NANODBC_TEXT("select * from test_exception where i = -10;"));

        REQUIRE(results.next());
        REQUIRE_THROWS_AS(results.get<nanodbc::date>(0), nanodbc::type_incompatible_error);
        REQUIRE_THROWS_AS(results.get<nanodbc::timestamp>(0), nanodbc::type_incompatible_error);

        results =
            execute(connection, NANODBC_TEXT("select * from test_exception where i is null;"));

        REQUIRE(results.next());
        REQUIRE_THROWS_AS(results.get<int>(0), nanodbc::null_access_error);
        REQUIRE_THROWS_AS(results.get<int>(42), nanodbc::index_range_error);

        nanodbc::statement statement(connection);
        REQUIRE(statement.open());
        REQUIRE(statement.connected());
        statement.close();
        REQUIRE_THROWS_AS(
            statement.prepare(NANODBC_TEXT("select * from test_exception;")),
            nanodbc::programming_error);
    }

    void test_execute_multiple()
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

    void test_execute_multiple_transaction()
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

    void test_get_info()
    {
        // A generic test to exercise the DBMS info API is callable.
        // DBMS-specific test (MySQL, SQLite, etc.) may perform extended checks.
        nanodbc::connection connection = connect();
        REQUIRE(!connection.get_info<nanodbc::string>(SQL_DRIVER_NAME).empty());
        REQUIRE(!connection.get_info<nanodbc::string>(SQL_ODBC_VER).empty());

        // Test SQLUSMALLINT results
        REQUIRE(connection.get_info<unsigned short>(SQL_NON_NULLABLE_COLUMNS) == SQL_NNC_NON_NULL);

        // Test SQUINTEGER results
        REQUIRE(connection.get_info<uint32_t>(SQL_ODBC_INTERFACE_CONFORMANCE) > 0);

        // get_info is robust to a SQUINTEGER InfoType being retrieved
        // as uint64_t
        REQUIRE(
            connection.get_info<uint64_t>(SQL_ODBC_INTERFACE_CONFORMANCE) ==
            connection.get_info<uint32_t>(SQL_ODBC_INTERFACE_CONFORMANCE));

        // Test SQUINTEGER bitmask results
        REQUIRE((connection.get_info<uint32_t>(SQL_CREATE_TABLE) & SQL_CT_CREATE_TABLE));

        // Test SQLULEN results
        REQUIRE(connection.get_info<uint64_t>(SQL_DRIVER_HDBC) > 0);
    }

    void test_implementation_row_descriptor()
    {
        auto c = connect();
        drop_table(c, NANODBC_TEXT("v_t1_t2"), true);
        drop_table(c, NANODBC_TEXT("t1"));
        drop_table(c, NANODBC_TEXT("t2"));

        create_table(c, NANODBC_TEXT("t1"), NANODBC_TEXT(R"(
t1_fid1 int NOT NULL,
t1_fid2 int NOT NULL,
name varchar(60) NOT NULL,
PRIMARY KEY(t1_fid1, t1_fid2)
)"));
        execute(
            c,
            NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (1,10,'John Malkovich');"));
        execute(
            c, NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (2,20,'Gina Bellman');"));
        execute(
            c, NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (3,30,'Bruce Willis');"));
        create_table(c, NANODBC_TEXT("t2"), NANODBC_TEXT(R"(
t2_fid int NOT NULL,
age int NOT NULL,
PRIMARY KEY(t2_fid)
)"));
        execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (1,53)"));
        execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (2,41)"));
        execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (3,60)"));

        // table
        {
            nanodbc::string sql =
                NANODBC_TEXT("SELECT t1_fid1 AS fid1, t1_fid2, name FROM t1 AS t");
            if (vendor_ == database_vendor::sqlserver)
                sql +=
                    NANODBC_TEXT(" FOR BROWSE"); // attributes like table name available only for
                                                 // SELECT statements containing FOR BROWSE clause

            nanodbc::statement s(c, sql);
            nanodbc::implementation_row_descriptor ird(s);
            REQUIRE(ird.count() == 3);
            REQUIRE(ird.count() == ird.count());
            for (short i = 0; i < ird.count(); i++)
            {
                if (vendor_ == database_vendor::mysql)
                {
                    REQUIRE(ird.catalog_name(i) != NANODBC_TEXT(""));
                    REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
                }
                else if (vendor_ == database_vendor::postgresql)
                {
                    if (i == 0) // NOTICE: fid1 alias has no catalog!(?)
                        REQUIRE(ird.catalog_name(i) == NANODBC_TEXT(""));
                    else
                        REQUIRE(ird.catalog_name(i) != NANODBC_TEXT(""));
                    REQUIRE(ird.schema_name(i) == NANODBC_TEXT("public"));
                }
                else if (vendor_ == database_vendor::sqlite)
                {
                    REQUIRE(ird.catalog_name(i) == NANODBC_TEXT("main"));
                    REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
                }
                else if (vendor_ == database_vendor::sqlserver)
                {
                    REQUIRE(ird.catalog_name(i) != NANODBC_TEXT(""));
                    REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
                }
                else
                {
                    REQUIRE(ird.schema_name(i) != NANODBC_TEXT(""));
                    REQUIRE(ird.schema_name(i) != NANODBC_TEXT(""));
                }
            }
            // t1_fid1
            REQUIRE(!ird.auto_unique_value(0));
            if (vendor_ == database_vendor::sqlite)
                REQUIRE(ird.base_column_name(0) == NANODBC_TEXT("fid1"));
            else
                REQUIRE(ird.base_column_name(0) == NANODBC_TEXT("t1_fid1"));
            REQUIRE(ird.name(0) == NANODBC_TEXT("fid1"));
            REQUIRE(ird.base_table_name(0) == NANODBC_TEXT("t1"));
            if (vendor_ == database_vendor::mysql)
                REQUIRE(ird.table_name(0) == NANODBC_TEXT("t"));
            else
                REQUIRE(ird.table_name(0) == NANODBC_TEXT("t1"));
            // t1_fid2
            REQUIRE(!ird.auto_unique_value(1));
            REQUIRE(ird.base_column_name(1) == NANODBC_TEXT("t1_fid2"));
            REQUIRE(ird.name(1) == NANODBC_TEXT("t1_fid2"));
            REQUIRE(ird.base_table_name(1) == NANODBC_TEXT("t1"));
            if (vendor_ == database_vendor::mysql)
                REQUIRE(ird.table_name(1) == NANODBC_TEXT("t"));
            else
                REQUIRE(ird.table_name(1) == NANODBC_TEXT("t1"));
            // name
            REQUIRE(ird.base_column_name(2) == NANODBC_TEXT("name"));
            REQUIRE(ird.name(2) == NANODBC_TEXT("name"));
            REQUIRE(ird.base_table_name(2) == NANODBC_TEXT("t1"));
            if (vendor_ == database_vendor::mysql)
                REQUIRE(ird.table_name(2) == NANODBC_TEXT("t"));
            else
                REQUIRE(ird.table_name(2) == NANODBC_TEXT("t1"));
        }

        create_table(
            c,
            NANODBC_TEXT("v_t1_t2"),
            NANODBC_TEXT("SELECT t1.*, t2.* FROM t1 INNER JOIN t2 ON t1.t1_fid1 = t2.t2_fid"),
            true);

        // view
        {
            nanodbc::string sql = NANODBC_TEXT("SELECT t1_fid1 AS fid1, t1_fid2 AS fid2, t2_fid AS "
                                               "fid3, name AS n, age AS a FROM v_t1_t2");
            if (vendor_ == database_vendor::sqlserver)
                sql +=
                    NANODBC_TEXT(" FOR BROWSE"); // attributes like table name available only for
                                                 // SELECT statements containing FOR BROWSE clause

            nanodbc::statement s(c, sql);
            nanodbc::implementation_row_descriptor ird(s);
            REQUIRE(ird.count() == 5);
            // fid1
            if (vendor_ == database_vendor::sqlite)
                REQUIRE(ird.base_column_name(0) == NANODBC_TEXT("fid1"));
            else
                REQUIRE(ird.base_column_name(0) == NANODBC_TEXT("t1_fid1"));
            REQUIRE(ird.name(0) == NANODBC_TEXT("fid1"));
            if (vendor_ == database_vendor::mysql || vendor_ == database_vendor::postgresql)
            {
                REQUIRE(ird.base_table_name(0) == NANODBC_TEXT("v_t1_t2"));
                REQUIRE(ird.table_name(0) == NANODBC_TEXT("v_t1_t2"));
            }
            else
            {
                REQUIRE(ird.base_table_name(0) == NANODBC_TEXT("t1"));
                REQUIRE(ird.table_name(0) == NANODBC_TEXT("t1"));
            }
        }
    }

    void test_implementation_row_descriptor_with_expressions()
    {
        auto c = connect();
        nanodbc::string sql = NANODBC_TEXT("SELECT 'test' AS name, 2 + 3 AS age, 2 * 3");
        if (vendor_ == database_vendor::sqlserver)
            sql += NANODBC_TEXT(" FOR BROWSE"); // attributes like table name available only for
                                                // SELECT statements containing FOR BROWSE clause

        nanodbc::statement s(c, sql);
        nanodbc::implementation_row_descriptor ird(s);
        REQUIRE(ird.count() == 3);
        REQUIRE(ird.count() == ird.count());
        for (short i = 0; i < ird.count(); i++)
        {
            if (vendor_ == database_vendor::mysql)
            {
                REQUIRE(ird.catalog_name(i) != NANODBC_TEXT(""));
                REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
            }
            else if (vendor_ == database_vendor::postgresql)
            {
                REQUIRE(ird.catalog_name(i) == NANODBC_TEXT(""));
                REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
            }
            else if (vendor_ == database_vendor::sqlite)
            {
                REQUIRE(ird.catalog_name(i) == NANODBC_TEXT(""));
                REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
            }
            else if (vendor_ == database_vendor::sqlserver)
            {
                REQUIRE(ird.catalog_name(i) == NANODBC_TEXT(""));
                REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
            }
            else
            {
                REQUIRE(ird.schema_name(i) != NANODBC_TEXT(""));
                REQUIRE(ird.schema_name(i) != NANODBC_TEXT(""));
            }
        }
        // name
        REQUIRE(!ird.auto_unique_value(0));
        if (vendor_ == database_vendor::sqlite || vendor_ == database_vendor::postgresql)
            REQUIRE(ird.base_column_name(0) == NANODBC_TEXT("name"));
        else
            REQUIRE(ird.base_column_name(0) == NANODBC_TEXT(""));
        REQUIRE(ird.name(0) == NANODBC_TEXT("name"));
        REQUIRE(ird.base_table_name(0) == NANODBC_TEXT(""));
        REQUIRE(ird.table_name(0) == NANODBC_TEXT(""));
        // age
        REQUIRE(!ird.auto_unique_value(1));
        if (vendor_ == database_vendor::sqlite || vendor_ == database_vendor::postgresql)
            REQUIRE(ird.base_column_name(1) == NANODBC_TEXT("age"));
        else
            REQUIRE(ird.base_column_name(1) == NANODBC_TEXT(""));
        REQUIRE(ird.name(1) == NANODBC_TEXT("age"));
        REQUIRE(ird.base_table_name(1) == NANODBC_TEXT(""));
        REQUIRE(ird.table_name(1) == NANODBC_TEXT(""));
        // 2 * 3
        REQUIRE(!ird.auto_unique_value(2));
        if (vendor_ == database_vendor::mysql)
        {
            REQUIRE(ird.base_column_name(2) == NANODBC_TEXT(""));
            REQUIRE(ird.name(2) == NANODBC_TEXT("2 * 3"));
            REQUIRE(!ird.unnamed(2));
        }
        else if (vendor_ == database_vendor::postgresql)
        {
            REQUIRE(ird.base_column_name(2) == NANODBC_TEXT("?column?"));
            REQUIRE(ird.name(2) == NANODBC_TEXT("?column?"));
            REQUIRE(!ird.unnamed(2));
        }
        else if (vendor_ == database_vendor::sqlite)
        {
            REQUIRE(ird.base_column_name(2) == NANODBC_TEXT("2 * 3"));
            REQUIRE(ird.name(2) == NANODBC_TEXT("2 * 3"));
            REQUIRE_THROWS_WITH(!ird.unnamed(2), Catch::Contains("unsupported column attribute"));
        }
        else
        {
            REQUIRE(ird.base_column_name(2) == NANODBC_TEXT(""));
            REQUIRE(ird.name(2) == NANODBC_TEXT(""));
            REQUIRE(ird.unnamed(2));
        }
        REQUIRE(ird.base_table_name(2) == NANODBC_TEXT(""));
        REQUIRE(ird.table_name(2) == NANODBC_TEXT(""));
    }

    template <class T>
    void test_integral_template()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("test_integral"));
        execute(
            connection,
            NANODBC_TEXT("create table test_integral (i int, f float, d double precision);"));

        nanodbc::statement statement(connection);
        prepare(statement, NANODBC_TEXT("insert into test_integral (i, f, d) values (?, ?, ?);"));
        REQUIRE(statement.parameters() == 3);

        std::minstd_rand nanodbc_rand;
        const T i = nanodbc_rand() % 100; // also tests if bind(T) is defined
        const float f = nanodbc_rand() / (nanodbc_rand() + 1.0);
        const float d = -static_cast<std::int_fast32_t>(nanodbc_rand()) / (nanodbc_rand() + 1.0);

        short p = 0;
        statement.bind(p++, &i);
        statement.bind(p++, &f);
        statement.bind(p++, &d);

        REQUIRE(statement.connected());
        execute(statement);

        nanodbc::result results = execute(connection, NANODBC_TEXT("select * from test_integral;"));
        REQUIRE(results.next());

        // NOTE: Parentheses around REQIURE() expressions are to silence error:
        //       suggest parentheses around comparison in operand of '==' [-Werror=parentheses]
        T ref = {};
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
            Fixture fixture = {};
            using type = typename std::tuple_element<i, TypeList>::type;
            fixture.template test_integral_template<type>();
            fixture.template test_decimal_to_integral_conversion_template<type>();
            foreach
                <Fixture, TypeList, i - 1>::run();
        }
    };

    template <class Fixture, class TypeList>
    struct foreach<Fixture, TypeList, 0>
    {
        static void run()
        {
            Fixture fixture = {};
            using type = typename std::tuple_element<0, TypeList>::type;
            fixture.template test_integral_template<type>();
            fixture.template test_decimal_to_integral_conversion_template<type>();
        }
    };

    template <class Fixture>
    void test_integral()
    {
        foreach
            <Fixture, integral_test_types>::run();
    }

    void test_move()
    {
        nanodbc::connection orig_connection = connect();
        drop_table(orig_connection, NANODBC_TEXT("test_move"));
        execute(orig_connection, NANODBC_TEXT("create table test_move (i int);"));
        execute(orig_connection, NANODBC_TEXT("insert into test_move values (10);"));

        nanodbc::connection new_connection = std::move(orig_connection);
        execute(new_connection, NANODBC_TEXT("insert into test_move values (30);"));
        execute(new_connection, NANODBC_TEXT("insert into test_move values (20);"));

        nanodbc::result orig_results =
            execute(new_connection, NANODBC_TEXT("select i from test_move order by i desc;"));
        REQUIRE(orig_results.next());
        REQUIRE(orig_results.get<int>(0) == 30);
        REQUIRE(orig_results.next());
        REQUIRE(orig_results.get<int>(0) == 20);

        nanodbc::result new_results = std::move(orig_results);
        REQUIRE(new_results.next());
        REQUIRE(new_results.get<int>(0) == 10);
    }

    void test_null()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("test_null"));
        execute(connection, NANODBC_TEXT("create table test_null (a int, b varchar(10));"));

        nanodbc::statement statement(connection);

        prepare(statement, NANODBC_TEXT("insert into test_null (a, b) values (?, ?);"));
        REQUIRE(statement.parameters() == 2);
        statement.bind_null(0);
        statement.bind_null(1);
        execute(statement);

        prepare(statement, NANODBC_TEXT("insert into test_null (a, b) values (?, ?);"));
        REQUIRE(statement.parameters() == 2);
        statement.bind_null(0, 2);
        statement.bind_null(1, 2);
        execute(statement, 2);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select a, b from test_null order by a;"));

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));
        REQUIRE(results.get<int>(0, -1) == -1);
        REQUIRE(results.get<int>(1, -2) == -2);

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));
        REQUIRE(results.get<int>(0, -3) == -3);
        REQUIRE(results.get<int>(1, -4) == -4);

        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
        REQUIRE(results.is_null(1));
        REQUIRE(results.get<int>(0, -5) == -5);
        REQUIRE(results.get<int>(1, -6) == -6);

        REQUIRE(!results.next());
    }

    // checks that:         statement.bind(0, &i, 1, nullptr, nanodbc::statement::PARAM_IN);
    // works the same as:   statement.bind(0, &i, 1, nanodbc::statement::PARAM_IN);
    void test_nullptr_nulls()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_nullptr_nulls"));
        execute(connection, NANODBC_TEXT("create table test_nullptr_nulls (i int);"));

        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_nullptr_nulls (i) values (?);"));
            REQUIRE(statement.parameters() == 1);

            int i = 5;
            statement.bind(0, &i, 1, nullptr, nanodbc::statement::PARAM_IN);

            REQUIRE(statement.connected());
            execute(statement);

            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from test_nullptr_nulls;"));
            REQUIRE(results.next());

            REQUIRE(results.get<int>(0) == i);
        }

        execute(connection, NANODBC_TEXT("DELETE FROM test_nullptr_nulls;"));

        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_nullptr_nulls (i) values (?);"));
            REQUIRE(statement.parameters() == 1);

            int i = 5;
            statement.bind(0, &i, 1, nanodbc::statement::PARAM_IN);

            REQUIRE(statement.connected());
            execute(statement);

            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from test_nullptr_nulls;"));
            REQUIRE(results.next());

            REQUIRE(results.get<int>(0) == i);
        }
    }

    void test_null_with_bound_columns_unbound()
    {
        nanodbc::connection conn = connect();
        create_table(
            conn,
            NANODBC_TEXT("test_null_with_bound_columns_unbound"),
            NANODBC_TEXT("(a int, b float)"));

        nanodbc::statement statement(conn);
        prepare(
            statement,
            NANODBC_TEXT("insert into test_null_with_bound_columns_unbound (a, b) values (?, ?);"));
        REQUIRE(statement.parameters() == 2);
        statement.bind_null(0);
        statement.bind_null(1);
        execute(statement, 2);

        nanodbc::result results = execute(
            conn,
            NANODBC_TEXT("select a, b from test_null_with_bound_columns_unbound order by a;"));
        results.unbind();

        REQUIRE(results.next());

        if (vendor_ == database_vendor::mysql)
            REQUIRE(results.is_null(0)); // MySQL: Bug or non-standard behaviour? SQLBindCol sets
                                         // the indicator to SQL_NULL_DATA
        else
            REQUIRE(!results.is_null(0)); // false as undetermined until SQLGetData is called
        REQUIRE(results.get<int>(0, -1) == -1);
        REQUIRE(results.is_null(0)); // determined

        if (vendor_ == database_vendor::mysql)
            REQUIRE(results.is_null(1)); // MySQL: Bug or non-standard behaviour? SQLBindCol sets
                                         // the indicator to SQL_NULL_DATA
        else
            REQUIRE(!results.is_null(1)); // false as undetermined until SQLGetData is called
        REQUIRE(results.get<double>(1, 1.23) >= 1.23);
        REQUIRE(results.is_null(1)); // determined
    }

    void test_result_at_end()
    {
        nanodbc::connection connection = connect();

        // Test empty result (current position == number of rows == 0)
        {
            create_table(connection, NANODBC_TEXT("test_result_at_end"), NANODBC_TEXT("(i int)"));
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from test_result_at_end;"));
            REQUIRE(results.at_end());
            REQUIRE(!results.next());
            REQUIRE(results.at_end());
        }
        // Test non-empty result, but not moved at first
        {
            create_table(connection, NANODBC_TEXT("test_result_at_end"), NANODBC_TEXT("(i int)"));
            execute(connection, NANODBC_TEXT("insert into test_result_at_end values (1);"));
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from test_result_at_end;"));
            REQUIRE(results.at_end());
        }
        // Test non-empty result and moved at first
        {
            create_table(connection, NANODBC_TEXT("test_result_at_end"), NANODBC_TEXT("(i int)"));
            execute(connection, NANODBC_TEXT("insert into test_result_at_end values (1);"));
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select * from test_result_at_end;"));
            REQUIRE(results.next());
            REQUIRE(results.at_end());
            REQUIRE(!results.next());
            REQUIRE(results.at_end());
        }
    }

    void test_result_iterator()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_result_iterator"));
        execute(
            connection, NANODBC_TEXT("create table test_result_iterator (i int, s varchar(10));"));
        execute(connection, NANODBC_TEXT("insert into test_result_iterator values (1, 'one');"));
        execute(connection, NANODBC_TEXT("insert into test_result_iterator values (2, 'two');"));
        execute(connection, NANODBC_TEXT("insert into test_result_iterator values (3, 'tri');"));

        // Test standard algorithm
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from test_result_iterator;"));
            REQUIRE(std::distance(begin(results), end(results)) == 3);
        }

        // Test classic for loop iteration
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from test_result_iterator;"));
            for (auto it = begin(results); it != end(results); ++it)
            {
                REQUIRE(it->get<int>(0) > 0);
                REQUIRE(it->get<nanodbc::string>(1).size() == 3);
            }
            REQUIRE(
                std::distance(begin(results), end(results)) ==
                0); // InputIterators only guarantee validity for single pass algorithms
        }

        // Test range-based for loop iteration
        {
            nanodbc::result results =
                execute(connection, NANODBC_TEXT("select i, s from test_result_iterator;"));
            for (auto& row : results)
            {
                REQUIRE(row.get<int>(0) > 0);
                REQUIRE(row.get<nanodbc::string>(1).size() == 3);
            }
            REQUIRE(
                std::distance(begin(results), end(results)) ==
                0); // InputIterators only guarantee validity for single pass algorithms
        }
    }

    void test_statement_usable_when_result_gone()
    {
        // https://github.com/nanodbc/nanodbc/issues/333
        auto c = connect();

        create_table(
            c,
            NANODBC_TEXT("test_statement_usable_when_result_gone"),
            NANODBC_TEXT("(a int, b varchar(10))"));
        execute(
            c,
            NANODBC_TEXT("insert into test_statement_usable_when_result_gone values (1, 'one');"));
        execute(
            c,
            NANODBC_TEXT("insert into test_statement_usable_when_result_gone values (2, 'two');"));
        execute(
            c,
            NANODBC_TEXT("insert into test_statement_usable_when_result_gone values (3, 'tri');"));

        nanodbc::statement s;
        s.prepare(
            c, NANODBC_TEXT("select a, b from test_statement_usable_when_result_gone order by a;"));

        auto process_data_and_throw = [](int i)
        {
            if (i == 2)
                throw std::runtime_error("a==2");
        };

        try
        {
            nanodbc::result r = s.execute();
            while (r.next())
                process_data_and_throw(r.get<int>(0));
            REQUIRE(false); // should never get here
        }
        catch (std::runtime_error const& e)
        {
            REQUIRE_THAT(e.what(), Catch::Contains("a==2"));
            REQUIRE(s.open());
            nanodbc::result r = s.execute(); // statement remains usable, re-execute it
            int sum = 0;
            while (r.next())
                sum += r.get<int>(0);
            REQUIRE(sum == 1 + 2 + 3);
        }
    }

    void test_simple()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        drop_table(connection, NANODBC_TEXT("test_simple"));
        execute(
            connection,
            NANODBC_TEXT("create table test_simple (sort_order int, a int, b varchar(10));"));
        execute(connection, NANODBC_TEXT("insert into test_simple values (2, 1, 'one');"));
        execute(connection, NANODBC_TEXT("insert into test_simple values (3, 2, 'two');"));
        execute(connection, NANODBC_TEXT("insert into test_simple values (4, 3, 'tri');"));
        execute(connection, NANODBC_TEXT("insert into test_simple values (1, NULL, 'z');"));

        {
            nanodbc::result results = execute(
                connection, NANODBC_TEXT("select a, b from test_simple order by sort_order;"));
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
            if (affected == 0)
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
            REQUIRE(results.get<nanodbc::string>(0, NANODBC_TEXT("null")) == NANODBC_TEXT("null"));
            REQUIRE(
                results.get<nanodbc::string>(NANODBC_TEXT("a"), NANODBC_TEXT("null")) ==
                NANODBC_TEXT("null"));
            REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("z"));
            REQUIRE(results.get<nanodbc::string>(NANODBC_TEXT("b")) == NANODBC_TEXT("z"));

            int ref_int;
            results.get_ref(0, -1, ref_int);
            REQUIRE(ref_int == -1);
            results.get_ref(NANODBC_TEXT("a"), -2, ref_int);
            REQUIRE(ref_int == -2);

            nanodbc::string ref_str;
            results.get_ref<nanodbc::string>(0, NANODBC_TEXT("null"), ref_str);
            REQUIRE(ref_str == NANODBC_TEXT("null"));
            results.get_ref<nanodbc::string>(NANODBC_TEXT("a"), NANODBC_TEXT("null2"), ref_str);
            REQUIRE(ref_str == NANODBC_TEXT("null2"));

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results.next());
            // row = 1|one
            // .....................................................................................
            REQUIRE(results.get<int>(0) == 1);
            REQUIRE(results.get<int>(NANODBC_TEXT("a")) == 1);
            REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("one"));
            REQUIRE(results.get<nanodbc::string>(NANODBC_TEXT("b")) == NANODBC_TEXT("one"));

            nanodbc::result results_copy = results;

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results_copy.next());
            // row = 2|two
            // .....................................................................................
            REQUIRE(results_copy.get<int>(0, -1) == 2);
            REQUIRE(results_copy.get<int>(NANODBC_TEXT("a"), -1) == 2);
            REQUIRE(results_copy.get<nanodbc::string>(1) == NANODBC_TEXT("two"));
            REQUIRE(results_copy.get<nanodbc::string>(NANODBC_TEXT("b")) == NANODBC_TEXT("two"));

            // FIXME: not supported by the default SQL_CURSOR_FORWARD_ONLY
            // and will require SQL_ATTR_CURSOR_TYPE set to SQL_CURSOR_STATIC at least.
            // REQUIRE(results.position());

            nanodbc::result().swap(results_copy);

            // :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            REQUIRE(results.next());
            // row = 3|tri
            // .....................................................................................
            REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("3"));
            REQUIRE(results.get<nanodbc::string>(NANODBC_TEXT("a")) == NANODBC_TEXT("3"));
            REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("tri"));
            REQUIRE(results.get<nanodbc::string>(NANODBC_TEXT("b")) == NANODBC_TEXT("tri"));

            REQUIRE(!results.next());
            REQUIRE(results.at_end());
        }

        nanodbc::connection connection_copy(connection);

        connection.disconnect();
        REQUIRE(!connection.connected());
        REQUIRE(!connection_copy.connected());
    }

    void test_string()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const nanodbc::string name = NANODBC_TEXT("Fred");

        drop_table(connection, NANODBC_TEXT("test_string"));
        execute(connection, NANODBC_TEXT("create table test_string (s varchar(10));"));

        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into test_string(s) values(?)"));
        REQUIRE(query.parameters() == 1);
        query.bind(0, name.c_str());
        nanodbc::execute(query);

        nanodbc::result results = execute(connection, NANODBC_TEXT("select s from test_string;"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Fred"));

        nanodbc::string ref;
        results.get_ref(0, ref);
        REQUIRE(ref == name);
    }

#ifdef NANODBC_HAS_STD_OPTIONAL
    void test_string_optional()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const nanodbc::string name = NANODBC_TEXT("Fred");

        drop_table(connection, NANODBC_TEXT("test_string_optional"));
        execute(connection, NANODBC_TEXT("create table test_string_optional (s varchar(10));"));

        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into test_string_optional(s) values(?)"));
        REQUIRE(query.parameters() == 1);
        query.bind(0, name.c_str());
        nanodbc::execute(query);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select s from test_string_optional;"));
        REQUIRE(results.next());
        REQUIRE(*results.get<std::optional<nanodbc::string>>(0) == NANODBC_TEXT("Fred"));

        std::optional<nanodbc::string> ref;
        results.get_ref(0, ref);
        REQUIRE(*ref == name);
    }
#endif
    void test_string_with_varchar_max()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_string_with_varchar_max"));
        execute(
            connection,
            NANODBC_TEXT("create table test_string_with_varchar_max (s varchar(max));"));
        execute(
            connection,
            NANODBC_TEXT("insert into test_string_with_varchar_max(s) ")
                NANODBC_TEXT("values (REPLICATE(CAST(\'a\' AS varchar(MAX)), 15000))"));

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select s from test_string_with_varchar_max;"));
        REQUIRE(results.next());

        nanodbc::string select;
        results.get_ref(0, select);
        REQUIRE(select.size() == 15000);
    }

    void test_string_vector()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const std::vector<nanodbc::string> first_name = {
            NANODBC_TEXT("Fred"), NANODBC_TEXT("Barney"), NANODBC_TEXT("Dino")};
        const std::vector<nanodbc::string> last_name = {
            NANODBC_TEXT("Flintstone"), NANODBC_TEXT("Rubble"), NANODBC_TEXT("")};
        const std::vector<nanodbc::string> gender = {
            NANODBC_TEXT("Male"), NANODBC_TEXT("Male"), NANODBC_TEXT("")};

        drop_table(connection, NANODBC_TEXT("test_string_vector"));
        execute(
            connection,
            NANODBC_TEXT("create table test_string_vector (first varchar(10), last "
                         "varchar(10), gender varchar(10));"));

        nanodbc::statement query(connection);
        prepare(
            query,
            NANODBC_TEXT("insert into test_string_vector(first, last, gender) values(?, ?, ?)"));
        REQUIRE(query.parameters() == 3);

        // Without nulls
        query.bind_strings(0, first_name);

        // With null vector
        bool nulls[3] = {false, false, true};
        query.bind_strings(1, last_name, nulls);

        // With null sentry
        query.bind_strings(2, gender, NANODBC_TEXT(""));

        nanodbc::execute(query, 3);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select first,last,gender from test_string_vector"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Fred"));
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("Flintstone"));
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Barney"));
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("Rubble"));
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Dino"));
        REQUIRE(results.is_null(1));
        REQUIRE(results.is_null(2));
    }

    void test_string_view_vector()
    {
// std::string_view is only supported since C++17 compliant compilers
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const std::vector<nanodbc::string> first_name_str = {
            NANODBC_TEXT("Fred"), NANODBC_TEXT("Barney"), NANODBC_TEXT("Dino")};
        const std::vector<nanodbc::string> last_name_str = {
            NANODBC_TEXT("Flintstone"), NANODBC_TEXT("Rubble"), NANODBC_TEXT("")};
        const std::vector<nanodbc::string> gender_str = {
            NANODBC_TEXT("Male"), NANODBC_TEXT("Male"), NANODBC_TEXT("")};

        const std::vector<nanodbc::string_view> first_name(
            first_name_str.begin(), first_name_str.end());
        const std::vector<nanodbc::string_view> last_name(
            last_name_str.begin(), last_name_str.end());
        const std::vector<nanodbc::string_view> gender(gender_str.begin(), gender_str.end());

        drop_table(connection, NANODBC_TEXT("test_string_view_vector"));
        execute(
            connection,
            NANODBC_TEXT("create table test_string_view_vector (first varchar(10), last "
                         "varchar(10), gender varchar(10));"));

        nanodbc::statement query(connection);
        prepare(
            query,
            NANODBC_TEXT(
                "insert into test_string_view_vector(first, last, gender) values(?, ?, ?)"));
        REQUIRE(query.parameters() == 3);

        // Without nulls
        query.bind_strings(0, first_name);

        // With null vector
        bool nulls[3] = {false, false, true};
        query.bind_strings(1, last_name, nulls);

        // With null sentry
        query.bind_strings(2, gender, NANODBC_TEXT(""));

        nanodbc::execute(query, 3);

        nanodbc::result results = execute(
            connection, NANODBC_TEXT("select first,last,gender from test_string_view_vector"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Fred"));
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("Flintstone"));
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Barney"));
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("Rubble"));
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("Male"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("Dino"));
        REQUIRE(results.is_null(1));
        REQUIRE(results.is_null(2));
#endif //__cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
    }

    void test_string_vector_null_vector()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        const std::vector<nanodbc::string> name = {NANODBC_TEXT("foo"), NANODBC_TEXT("bar")};

        drop_table(connection, NANODBC_TEXT("test_string_vector_null_vector"));
        execute(
            connection,
            NANODBC_TEXT("create table test_string_vector_null_vector (name varchar(10));"));

        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into test_string_vector_null_vector(name) values(?)"));
        REQUIRE(query.parameters() == 1);

        // With null vector, we need to use `std::vector<uint8_t>` instead of
        // `std::vector<bool>` because the latter is a space efficient
        // specialization that does not have a `data()` method.
        std::vector<uint8_t> nulls = {false, false, true};
        query.bind_strings(1, name, reinterpret_cast<bool*>(nulls.data()));

        nanodbc::execute(query, 3);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select name from test_string_vector_null_vector"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("foo"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("bar"));
        REQUIRE(results.next());
        REQUIRE(results.is_null(0));
    }

    void test_batch_binary()
    {
        nanodbc::connection connection = connect();
        REQUIRE(connection.native_dbc_handle() != nullptr);
        REQUIRE(connection.native_env_handle() != nullptr);
        REQUIRE(connection.transactions() == std::size_t(0));

        // Include null termination to ensure they are handled properly
        const std::vector<std::vector<uint8_t>> data = {
            {0x00, 0x01, 0x02, 0x03}, {0x03, 0x02, 0x01, 0x00}};

        drop_table(connection, NANODBC_TEXT("test_batch_binary"));
        nanodbc::string const binary_type_name = get_binary_type_name();

        // PostgreSQL does not allow limits on bytea fields, MS-SQL requires
        // them on varbinary fields
        nanodbc::string create_table_sql = NANODBC_TEXT("create table test_batch_binary (s ");
        if (vendor_ == database_vendor::postgresql)
        {
            create_table_sql = create_table_sql + binary_type_name + NANODBC_TEXT(")");
        }
        else
        {
            create_table_sql = create_table_sql + binary_type_name + NANODBC_TEXT("(10))");
        }

        execute(connection, create_table_sql);
        nanodbc::statement query(connection);
        prepare(query, NANODBC_TEXT("insert into test_batch_binary(s) values(?)"));
        REQUIRE(query.parameters() == 1);
        query.bind(0, data);
        nanodbc::execute(query, 2);

        nanodbc::result results =
            execute(connection, NANODBC_TEXT("select s from test_batch_binary;"));
        REQUIRE(results.next());
        REQUIRE(results.get<std::vector<uint8_t>>(0) == data[0]);
        REQUIRE(results.next());
        REQUIRE(results.get<std::vector<uint8_t>>(0) == data[1]);
    }

    void test_time()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("test_time"), NANODBC_TEXT("t time"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_time(t) values (?);"));
            REQUIRE(statement.parameters() == 1);

            nanodbc::time t{11, 45, 59};
            statement.bind(0, &t);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select t from test_time;"));
            REQUIRE(result.next());
            auto t = result.get<nanodbc::time>(0);
            REQUIRE(t.hour == 11);
            REQUIRE(t.min == 45);
            REQUIRE(t.sec == 59);
        }
    }

#ifdef NANODBC_HAS_STD_OPTIONAL
    void test_time_optional()
    {
        auto connection = connect();
        create_table(connection, NANODBC_TEXT("test_time_optional"), NANODBC_TEXT("t time"));

        // insert
        {
            nanodbc::statement statement(connection);
            prepare(statement, NANODBC_TEXT("insert into test_time_optional(t) values (?);"));
            REQUIRE(statement.parameters() == 1);

            nanodbc::time t{11, 45, 59};
            statement.bind(0, &t);
            execute(statement);
        }

        // select
        {
            auto result = execute(connection, NANODBC_TEXT("select t from test_time_optional;"));
            REQUIRE(result.next());
            auto t = result.get<std::optional<nanodbc::time>>(0);
            REQUIRE((*t).hour == 11);
            REQUIRE((*t).min == 45);
            REQUIRE((*t).sec == 59);
        }
    }
#endif
    void test_transaction()
    {
        nanodbc::connection connection = connect();

        drop_table(connection, NANODBC_TEXT("test_transaction"));
        if (vendor_ == database_vendor::mysql)
            execute(
                connection, NANODBC_TEXT("create table test_transaction (i int) ENGINE = INNODB;"));
        else
            execute(connection, NANODBC_TEXT("create table test_transaction (i int);"));

        nanodbc::statement statement(connection);
        prepare(statement, NANODBC_TEXT("insert into test_transaction (i) values (?);"));
        REQUIRE(statement.parameters() == 1);

        static const int elements = 10;
        int data[elements] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        statement.bind(0, data, elements);
        execute(statement, elements);

        static const nanodbc::string::value_type* query =
            NANODBC_TEXT("select count(1) from test_transaction;");

        check_rows_equal(execute(connection, query), 10);

        REQUIRE(connection.transactions() == 0);
        {
            nanodbc::transaction transaction(connection);
            REQUIRE(connection.transactions() == 1);
            execute(connection, NANODBC_TEXT("delete from test_transaction;"));
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
            execute(connection, NANODBC_TEXT("delete from test_transaction;"));
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
            execute(connection, NANODBC_TEXT("delete from test_transaction;"));
            check_rows_equal(execute(connection, query), 0);
            REQUIRE(connection.transactions() == 1);
            transaction.commit(); // performs actual commit and releases transaction
            REQUIRE(connection.transactions() == 0);
        }
        REQUIRE(connection.transactions() == 0);

        check_rows_equal(execute(connection, query), 0);
    }

#if defined(_MSC_VER)
    void test_win32_variant()
    {
        auto cn = connect();
        create_table(
            cn,
            NANODBC_TEXT("test_win32_variant"),
            NANODBC_TEXT("(i int, f float, d decimal(9, 3), dt date, t time, s varchar(60), b ") +
                get_binary_type_name(16) + NANODBC_TEXT(")"));

        // test data
        int i = 7;
        double f = 3.14159265359;
        double d = 3.141;
        nanodbc::date dt{2022, 4, 29};
        nanodbc::time t{19, 31, 13};
        nanodbc::string s = NANODBC_TEXT("this is text");
        // GUID: {7330F811-F47F-41BC-A4FF-E792D073F41F}
        // INT: 23885548255760334674942869530154890271
        std::vector<std::vector<std::uint8_t>> b = {from_hex("11f830737ff4bc41a4ffe792d073f41f")};

        // prepare test table
        {
            nanodbc::statement st(cn);
            prepare(
                st,
                NANODBC_TEXT(
                    "insert into test_win32_variant(i,f,d,dt,t,s,b) values (?,?,?,?,?,?,?);"));

            st.bind(0, &i);
            st.bind(1, &f);
            st.bind(2, &d);
            st.bind(3, &dt);
            st.bind(4, &t);
            st.bind_strings(5, s.c_str(), s.size(), 1);
            st.bind(6, b);
            execute(st);
        }

        auto rs = execute(cn, NANODBC_TEXT("select i,f,d,dt,t,s,b from test_win32_variant;"));
        rs.next();
        {
            auto v = rs.get<_variant_t>(0);
            REQUIRE(v.vt == VT_I8);
            REQUIRE(static_cast<int>(v) == i);
        }
        {
            auto v = rs.get<_variant_t>(1);
            REQUIRE(v.vt == VT_R8);
            REQUIRE(static_cast<double>(v) == Approx(f));
        }
        {
            auto v = rs.get<_variant_t>(2);
            if (vendor_ == database_vendor::sqlite)
            {
                REQUIRE(v.vt == VT_BSTR);
                REQUIRE(static_cast<_bstr_t>(v) == _bstr_t("3.141"));
            }
            else
            {
                REQUIRE(v.vt == VT_DECIMAL);
                REQUIRE(static_cast<double>(v) == Approx(d));
            }
        }
        {
            ::SYSTEMTIME st{
                static_cast<WORD>(dt.year),
                static_cast<WORD>(dt.month),
                0,
                static_cast<WORD>(dt.day),
                0,
                0,
                0,
                0};
            ::DATE date{0};
            REQUIRE(::SystemTimeToVariantTime(&st, &date));
            _variant_t v_date(date, VT_DATE);

            auto v = rs.get<_variant_t>(3);
            REQUIRE(v.vt == VT_DATE);
            REQUIRE(v == Approx(v_date));
        }
        {
            ::SYSTEMTIME st{
                0,
                0,
                0,
                0,
                static_cast<WORD>(t.hour),
                static_cast<WORD>(t.min),
                static_cast<WORD>(t.sec),
                0};
            ::DATE date{0};
            REQUIRE(::SystemTimeToVariantTime(&st, &date));
            _variant_t v_date(date, VT_DATE);

            auto v = rs.get<_variant_t>(4);
            REQUIRE(v.vt == VT_DATE);
            REQUIRE(v == Approx(v_date));
        }
        {
            auto v = rs.get<_variant_t>(5);
            REQUIRE(v.vt == VT_BSTR);
            REQUIRE(static_cast<_bstr_t>(v).length() == s.size());
            REQUIRE(static_cast<_bstr_t>(v) == _bstr_t(s.c_str()));
        }
        {
            auto v = rs.get<_variant_t>(6);
            REQUIRE(v.vt == (VT_ARRAY | VT_UI1));

            std::vector<std::uint8_t> b_from_v(b[0].size(), 0);
            {
                void* v_data = nullptr;
                REQUIRE(SUCCEEDED(::SafeArrayAccessData(v.parray, &v_data)));
                std::memcpy(&b_from_v[0], v_data, b_from_v.size());
                REQUIRE(SUCCEEDED(::SafeArrayUnaccessData(v.parray)));
            }
            REQUIRE(b_from_v == b[0]);
        }
    }

    void test_win32_variant_null()
    {
        auto cn = connect();

        nanodbc::string const binary_type_name = get_binary_type_name();
        REQUIRE(!binary_type_name.empty());
        nanodbc::string const text_type_name = get_text_type_name();
        REQUIRE(!text_type_name.empty());

        nanodbc::string const table_name(NANODBC_TEXT("test_win32_variant_null"));
        drop_table(cn, table_name);
        execute(
            cn,
            NANODBC_TEXT("create table ") + table_name + NANODBC_TEXT("(") +
                NANODBC_TEXT("c0 int NULL,") + NANODBC_TEXT("c1 smallint NULL,") +
                NANODBC_TEXT("c2 float NULL,") + NANODBC_TEXT("c3 decimal(9, 3) NULL,") +
                NANODBC_TEXT("c4 date NULL,") + // seems more portable than datetime (SQL
                                                // Server), timestamp (PostgreSQL, MySQL)
                NANODBC_TEXT("c5 varchar(60) NULL,") + NANODBC_TEXT("c6 varchar(120) NULL,") +
                NANODBC_TEXT("c7 ") + text_type_name + NANODBC_TEXT(" NULL,") +
                NANODBC_TEXT("c8 ") + binary_type_name + NANODBC_TEXT(" NULL);"));

        execute(
            cn,
            NANODBC_TEXT("insert into ") + table_name +
                NANODBC_TEXT(" values (NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);"));

        _variant_t v_null;
        v_null.ChangeType(VT_NULL);

        // default binding without NULL fallback
        {
            auto rs = execute(cn, NANODBC_TEXT("select * from ") + table_name);
            rs.next();
            for (short i = 0; i < rs.columns() - 2; i++)
            {
                REQUIRE_THROWS_AS(rs.get<_variant_t>(i), nanodbc::null_access_error);
            }
            // TODO: Do we want to make get_ref<T> consistent and throw null_access_error as
            // post-SQLGetData for unbound columns?
            auto c7 = rs.get<_variant_t>(rs.columns() - 2);
            auto c8 = rs.get<_variant_t>(rs.columns() - 1);
        }
        // default binding with NULL fallback balue
        {
            auto rs = execute(cn, NANODBC_TEXT("select * from ") + table_name);
            rs.next();
            for (short i = 0; i < rs.columns(); i++)
            {
                auto v = rs.get<_variant_t>(i, v_null);
                REQUIRE(v == v_null);
            }
        }
        // unbound without NULL fallback
        {
            auto rs = execute(cn, NANODBC_TEXT("select * from ") + table_name);
            rs.unbind();
            rs.next();
            for (short i = 0; i < rs.columns() - 2; i++)
            {
                // TODO: Do we want to make get_ref<T> consistent and throw null_access_error as
                // post-SQLGetData for unbound columns?
                auto v = rs.get<_variant_t>(i);
                REQUIRE(v == v_null);
            }
        }
        // unbound with NULL fallback
        {
            auto rs = execute(cn, NANODBC_TEXT("select * from ") + table_name);
            rs.unbind();
            rs.next();
            for (short i = 0; i < rs.columns() - 2; i++)
            {
                auto v = rs.get<_variant_t>(i, v_null);
                REQUIRE(v == v_null);
            }
        }
    }

    void test_win32_variant_null_literal()
    {
        auto cn = connect();

        // test data
        _variant_t v_null;
        v_null.ChangeType(VT_NULL);

        // bound
        {
            auto rs = execute(cn, NANODBC_TEXT("select NULL as n;"));
            rs.next();
            // TODO: Confirm which driver one is buggy or displays non-standard behaviour
            if (vendor_ != database_vendor::postgresql)
            {
                REQUIRE_THROWS_AS(rs.get<_variant_t>(0), nanodbc::null_access_error);
            }
            else
            {
                auto v = rs.get<_variant_t>(0);
                REQUIRE(v == v_null);
            }
        }
        {
            auto rs = execute(cn, NANODBC_TEXT("select NULL as n;"));
            rs.next();
            auto v = rs.get<_variant_t>(0, v_null);
            REQUIRE(v == v_null);
        }
        // unbound
        {
            auto rs = execute(cn, NANODBC_TEXT("select NULL as n;"));
            rs.unbind();
            rs.next();
            // TODO: Confirm which driver one is buggy or displays non-standard behaviour
            if (vendor_ == database_vendor::mysql)
            {
                REQUIRE_THROWS_AS(rs.get<_variant_t>(0), nanodbc::null_access_error);
            }
            else
            {
                auto v = rs.get<_variant_t>(0);
                REQUIRE(v == v_null);
            }
        }
    }

    void test_win32_variant_row_cached_result()
    {
        auto cn = connect();
        create_table(
            cn,
            NANODBC_TEXT("test_win32_variant_row_cached_result"),
            NANODBC_TEXT(
                "(i0 int, s1 varchar(60), f2 float, s3 varchar(60), d4 decimal(9, 3), b5 ") +
                get_binary_type_name(256) + NANODBC_TEXT(", dt6 date, s7 varchar(60), t8 time)"));

        for (short v : {0, 1, 2})
        {
            // test data
            int i = v;
            double f = 3.14159265359 + v;
            double d = 3.141 + v;
            nanodbc::date dt{2022, 4 + v, 12 - v};
            nanodbc::time t{19, 5 + v, 30 - v};
            nanodbc::string s1 =
                NANODBC_TEXT("this is s1 #") + nanodbc::test::convert(std::to_string(1 + v));
            nanodbc::string s3 =
                NANODBC_TEXT("this is s3 #") + nanodbc::test::convert(std::to_string(3 + v));
            nanodbc::string s7 =
                NANODBC_TEXT("this is s7 #") + nanodbc::test::convert(std::to_string(7 + v));
            std::vector<std::vector<std::uint8_t>> b5 = {
                from_hex("11f830737ff4bc41a4ffe792d073f41f")};

            nanodbc::statement st(cn);
            prepare(
                st,
                NANODBC_TEXT("insert into test_win32_variant_row_cached_result "
                             "(i0,s1,f2,s3,d4,b5,dt6,s7,t8) values "
                             "(?,?,?,?,?,?,?,?,?);"));
            st.bind(0, &i);
            st.bind_strings(1, s1.c_str(), s1.size(), 1);
            st.bind(2, &f);
            st.bind_strings(3, s3.c_str(), s3.size(), 1);
            st.bind(4, &d);
            st.bind(5, b5);
            st.bind(6, &dt);
            st.bind_strings(7, s7.c_str(), s7.size(), 1);
            st.bind(8, &t);
            execute(st);
        }

        nanodbc::variant_row_cached_result rs = execute(
            cn,
            NANODBC_TEXT("select i0,s1,f2,s3,d4,b5,dt6,s7,t8 from "
                         "test_win32_variant_row_cached_result;"));
        rs.unbind(); // allow interleaved bound and unbound columns access
        short row_count = 0;
        while (rs.next())
        {
            for (short column = 0; column < rs.columns(); column++)
            {
                REQUIRE(!rs.is_null(column));

                auto v = rs.get(column);
                REQUIRE(v.vt != VT_NULL);
                REQUIRE(v.vt != VT_EMPTY);

                _bstr_t s(v);
                if (v.vt != (VT_ARRAY | VT_UI1))
                    REQUIRE(s.length() != 0);
            }
            row_count++;
        }
        REQUIRE(row_count == 3);
    }

#endif // _MSC_VER

    void test_while_next_iteration()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_while_next_iteration"));
        execute(connection, NANODBC_TEXT("create table test_while_next_iteration (i int);"));
        execute(connection, NANODBC_TEXT("insert into test_while_next_iteration values (1);"));
        execute(connection, NANODBC_TEXT("insert into test_while_next_iteration values (2);"));
        execute(connection, NANODBC_TEXT("insert into test_while_next_iteration values (3);"));
        nanodbc::result results = execute(
            connection, NANODBC_TEXT("select * from test_while_next_iteration order by 1 desc;"));
        int i = 3;
        while (results.next())
        {
            REQUIRE(results.get<int>(0) == i--);
        }
    }

    void test_while_not_end_iteration()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_while_not_end_iteration"));
        execute(connection, NANODBC_TEXT("create table test_while_not_end_iteration (i int);"));
        execute(connection, NANODBC_TEXT("insert into test_while_not_end_iteration values (1);"));
        execute(connection, NANODBC_TEXT("insert into test_while_not_end_iteration values (2);"));
        execute(connection, NANODBC_TEXT("insert into test_while_not_end_iteration values (3);"));
        nanodbc::result results = execute(
            connection,
            NANODBC_TEXT("select * from test_while_not_end_iteration order by 1 desc;"));
        int i = 3;
        while (!results.at_end())
        {
            results.next();
            REQUIRE(results.get<int>(0) == i--);
        }
    }

    void test_batch_insert_describe_param()
    {
        std::size_t const batch_size = 9;
        int integers[batch_size] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        float floats[batch_size] = {1.123f, 2.345f, 3.1f, 4.5f, 5.678f, 6.f, 7.89f, 8.90f, 9.1234f};
        nanodbc::string::value_type trunc_float[batch_size][6] = {
            NANODBC_TEXT("1.100"),
            NANODBC_TEXT("2.300"),
            NANODBC_TEXT("3.100"),
            NANODBC_TEXT("4.500"),
            NANODBC_TEXT("5.700"),
            NANODBC_TEXT("6.000"),
            NANODBC_TEXT("7.900"),
            NANODBC_TEXT("8.900"),
            NANODBC_TEXT("9.100")};
        nanodbc::string::value_type strings[batch_size][60] = {
            NANODBC_TEXT("first string"),
            NANODBC_TEXT("second string"),
            NANODBC_TEXT("third string"),
            NANODBC_TEXT("this is fourth string"),
            NANODBC_TEXT("finally, the fifthstring"),
            NANODBC_TEXT(""),
            NANODBC_TEXT("sixth string"),
            NANODBC_TEXT("A"),
            NANODBC_TEXT("ninth string")};

        auto conn = connect();
        create_table(
            conn,
            NANODBC_TEXT("test_batch_insert_param_type"),
            NANODBC_TEXT("(i int, s varchar(60), f float, d decimal(9, 3))"));
        nanodbc::string insert(NANODBC_TEXT(
            "insert into test_batch_insert_param_type (i, s, f, d) values(?, ?, ?, ?)"));
        nanodbc::statement stmt(conn);
        prepare(stmt, insert);

        // Set parameter description for 2 of the four parameters
        // For the int, float, let nanodbc infer parameter meta data
        // during bind.
        std::vector<short> idx{1, 3};
        std::vector<short> type{12, 3};
        std::vector<unsigned long> size{60, 9};
        std::vector<short> scale{0, 1}; // round to one decimal
        stmt.describe_parameters(idx, type, size, scale);

        stmt.bind(0, integers, batch_size);
        stmt.bind_strings(1, strings);
        stmt.bind(2, floats, batch_size);
        stmt.bind(3, floats, batch_size);
        nanodbc::transact(stmt, batch_size);
        {
            auto result = nanodbc::execute(
                conn,
                NANODBC_TEXT("select i, f, s, d from test_batch_insert_param_type order by i asc"));
            std::size_t i = 0;
            while (result.next())
            {
                REQUIRE(result.get<int>(0) == integers[i]);
                REQUIRE(
                    result.get<float>(1) == floats[i]); // exact test might fail, switch to Approx
                REQUIRE(result.get<nanodbc::string>(2) == strings[i]);
                REQUIRE(result.get<nanodbc::string>(3) == trunc_float[i]);
                ++i;
            }
            REQUIRE(i == batch_size);
        }
    }

    void test_param_size_scale_type()
    {
        auto conn = connect();
        create_table(
            conn,
            NANODBC_TEXT("test_param_size_scale_type"),
            NANODBC_TEXT("(i int, s varchar(60), f float, d decimal(9, 3))"));
        nanodbc::string insert(
            NANODBC_TEXT("insert into test_param_size_scale_type (i, s, f, d) values(?, ?, ?, ?)"));
        nanodbc::statement stmt(conn);
        prepare(stmt, insert);

        // size: For numeirc maximum number of digits
        REQUIRE(stmt.parameter_size(0) == 10);
        REQUIRE(stmt.parameter_type(0) == SQL_INTEGER);
        REQUIRE(stmt.parameter_scale(0) == 0);

        // size: number of characters
        REQUIRE(stmt.parameter_size(1) == 60);
        REQUIRE(stmt.parameter_type(1) == SQL_VARCHAR);
        REQUIRE(stmt.parameter_scale(1) == 0);

        REQUIRE(stmt.parameter_size(2) == 53);
        REQUIRE(stmt.parameter_type(2) == SQL_FLOAT);
        REQUIRE(stmt.parameter_scale(2) == 0);

        REQUIRE(stmt.parameter_size(3) == 9);
        REQUIRE(stmt.parameter_type(3) == SQL_DECIMAL);
        REQUIRE(stmt.parameter_scale(3) == 3);
    }

    void test_statement_prepare_reuse()
    {
        nanodbc::connection connection = connect();
        drop_table(connection, NANODBC_TEXT("test_statement_prepare_reuse"));
        execute(connection, NANODBC_TEXT("create table test_statement_prepare_reuse (i int);"));

        nanodbc::statement stmt(connection);
        nanodbc::prepare(
            stmt, NANODBC_TEXT("insert into test_statement_prepare_reuse values (?);"));
        for (int i = 0; i < 10; ++i)
        {
            stmt.bind(0, &i);
            nanodbc::result results = stmt.execute();
            REQUIRE(results.affected_rows() == 1);
        }
    }

    void test_std_optional()
    {
#ifdef NANODBC_HAS_STD_OPTIONAL
        test_batch_insert_integral_optional();
        test_string_optional();
        test_time_optional();
        test_date_optional();
#endif
    }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // NANODBC_TEST_CASE_FIXTURE_H
