#include "test_case_fixture.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <atlsafe.h>
#include <comutil.h>
#ifdef _DEBUG
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif
#endif

namespace
{
struct mssql_fixture : public test_case_fixture
{
    mssql_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_MSSQL");
    }
};
}

TEST_CASE_METHOD(mssql_fixture, "test_driver", "[mssql][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(mssql_fixture, "test_affected_rows", "[mssql][affected_rows]")
{
    // Skip on SQL Server 2008, see details at
    // http://help.appveyor.com/discussions/problems/4704-database-cannot-be-autostarted-during-server-shutdown-or-startup
    if (get_env("DB") == NANODBC_TEXT("MSSQL2008"))
    {
        WARN("test_affected_rows skipped on AppVeyor with SQL Server 2008");
        return;
    }

// Enable MARS required?
#if 0
    enum { SQL_COPT_SS_MARS_ENABLED = 1224, SQL_MARS_ENABLED_YES = 1 }; // sqlext.h
    int rc = ::SQLSetConnectAttr(conn.native_dbc_handle(), SQL_COPT_SS_MARS_ENABLED, (SQLPOINTER)SQL_MARS_ENABLED_YES, SQL_IS_UINTEGER);
    REQUIRE(rc == 0);
#endif

    auto conn = connect();
    auto const current_db_name = conn.database_name();

    // CREATE DATABASE|TABLE
    {
        execute(
            conn,
            NANODBC_TEXT(
                "IF DB_ID('nanodbc_test_temp_db') IS NOT NULL DROP DATABASE nanodbc_test_temp_db"));
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("CREATE DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.affected_rows() == -1);
        execute(conn, NANODBC_TEXT("USE nanodbc_test_temp_db"));
        result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
        REQUIRE(result.affected_rows() == -1);
    }
    // INSERT
    {
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (1)"));
        REQUIRE(result.affected_rows() == 1);
        result = execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (2)"));
        REQUIRE(result.affected_rows() == 1);
    }
    // SELECT
    {
        auto result = execute(conn, NANODBC_TEXT("SELECT i FROM nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == -1);
    }
    // DELETE
    {
        auto result = execute(conn, NANODBC_TEXT("DELETE FROM nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 2);
    }
    // DROP DATABASE|TABLE
    {
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("DROP TABLE nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == -1);
        execute(conn, NANODBC_TEXT("USE ") + current_db_name);
        result = execute(conn, NANODBC_TEXT("DROP DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.affected_rows() == -1);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_integral", "[mssql][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_string", "[mssql][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_mixed", "[mssql][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(mssql_fixture, "test_blob", "[mssql][blob][binary][varbinary]")
{
    nanodbc::connection connection = connect();
    // Test data size less than the default size of the internal buffer (1024)
    {
        create_table(connection, NANODBC_TEXT("test_blob"), NANODBC_TEXT("(data varbinary(max))"));
        execute(
            connection,
            NANODBC_TEXT("insert into test_blob values (CONVERT(varbinary(max), "
                         "'0x010100000000000000000059400000000000005940', 1));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob;"));
        REQUIRE(results.next());

        auto const blob = results.get<std::vector<std::uint8_t>>(0);
        REQUIRE(blob.size() == 21);
        REQUIRE(to_hex(blob) == "010100000000000000000059400000000000005940");
    }

    // Test data size greater than, but not multiple of, the default size of the internal buffer
    // (1024)
    {
        create_table(connection, NANODBC_TEXT("test_blob"), NANODBC_TEXT("(data varbinary(max))"));
        execute(connection, NANODBC_TEXT("insert into test_blob values (CRYPT_GEN_RANDOM(1579));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob;"));
        REQUIRE(results.next());
        REQUIRE(results.get<std::vector<std::uint8_t>>(0).size() == 1579);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_large_blob", "[mssql][blob][binary][varbinary]")
{
    std::vector<std::uint8_t> blob;
    {
        std::string filename{get_data_path("large_binary_object_geometry_wkb.txt")};
        auto const hex = read_text_file(filename);
        blob = from_hex(hex);
    }

    // Test executing prepared statement with size of blbo larger than max (eg. SQL Server 8000 Bytes)
    auto connection = connect();
    {
        create_table(connection, NANODBC_TEXT("test_large_blob"), NANODBC_TEXT("(data varbinary(max))"));
        nanodbc::statement stmt(connection);
        prepare(stmt, NANODBC_TEXT("INSERT INTO test_large_blob (data) VALUES (?)"));

        std::vector<std::vector<std::uint8_t>> rows = {blob};
        stmt.bind(0, rows);
        execute(stmt);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_large_blob_geometry", "[mssql][blob][binary][varbinary][geometry]")
{
    if (get_env("DB") == NANODBC_TEXT("MSSQL2008"))
    {
        WARN("test_large_blob_geometry skipped on AppVeyor with SQL Server 2008");
        return;
    }

    std::vector<std::uint8_t> blob;
    {
        std::string filename{get_data_path("large_binary_object_geometry_wkb.txt")};
        auto const hex = read_text_file(filename);
        blob = from_hex(hex);
    }

    // Test executing prepared statement with size of blbo larger than max (eg. SQL Server 8000 Bytes)
    auto connection = connect();
    {
        create_table(connection, NANODBC_TEXT("test_large_blob_geometry"), NANODBC_TEXT("(data GEOMETRY)"));
        nanodbc::statement stmt(connection);
        prepare(stmt, NANODBC_TEXT("INSERT INTO test_large_blob_geometry (data) VALUES (geometry::STGeomFromWKB(?, 0))"));

        std::vector<std::vector<std::uint8_t>> rows = {blob};
        stmt.bind(0, rows);
        execute(stmt);
    }
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_blob_with_varchar",
    "[mssql][blob][binary][varbinary][varchar]")
{
    nanodbc::string s = NANODBC_TEXT(
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
        "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"
        "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIII"
        "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIJJJJJJJJJJJ"
        "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJKKK"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "KKKKKKLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
        "LLLLLLLLLLLLLLLMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"
        "MMMMMMMMMMMMMMMMMMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
        "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
        "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"
        "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ"
        "QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
        "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRSSSSSSSSSSSSSSSSSSSSSSS"
        "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTTTTTT"
        "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTUUUUUU"
        "UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU"
        "UUUVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
        "VVVVVVVVVVVVWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
        "WWWWWWWWWWWWWWWWWWWWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"
        "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"
        "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

    nanodbc::connection connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_blob_with_varchar"), NANODBC_TEXT("(data varbinary(max))"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_blob_with_varchar values (CONVERT(varbinary(max), '") + s +
            NANODBC_TEXT("'));"));

    nanodbc::result results =
        nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob_with_varchar;"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == s);
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();

    // BLock Cursors: https://technet.microsoft.com/en-us/library/aa172590.aspx
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("test_variable_string"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (1, 'this is a shorter text');"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (2, 'this is a longer text of the two "
            "texts in the table');"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from test_variable_string order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(
        results.get<nanodbc::string>(1) ==
        NANODBC_TEXT("this is a longer text of the two texts in the table"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar_and_first_row_null",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("test_variable_string"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(conn, NANODBC_TEXT("insert into test_variable_string (i, s) values (1, NULL);"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (2, 'this is a longer text of the two "
            "texts in the table');"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from test_variable_string order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(
        results.get<nanodbc::string>(1) ==
        NANODBC_TEXT("this is a longer text of the two texts in the table"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar_and_second_row_null",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("test_variable_string"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (1, 'this is a shorter text');"));
    execute(conn, NANODBC_TEXT("insert into test_variable_string (i, s) values (2, NULL);"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from test_variable_string order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_list_catalogs", "[mssql][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_list_schemas", "[mssql][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_columns", "[mssql][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_primary_keys", "[mssql][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_tables", "[mssql][catalog][tables]")
{
    test_catalog_tables();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_table_privileges", "[mssql][catalog][tables]")
{
    test_catalog_table_privileges();
}

TEST_CASE_METHOD(mssql_fixture, "test_column_descriptor", "[mssql][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(mssql_fixture, "test_dbms_info", "[mssql][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(mssql_fixture, "test_get_info", "[mssql][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(mssql_fixture, "test_decimal_conversion", "[mssql][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(mssql_fixture, "test_exception", "[mssql][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_execute_multiple_transaction",
    "[mssql][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(mssql_fixture, "test_execute_multiple", "[mssql][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(mssql_fixture, "test_integral", "[mssql][integral]")
{
    test_integral<mssql_fixture>();
}

TEST_CASE_METHOD(mssql_fixture, "test_move", "[mssql][move]")
{
    test_move();
}

TEST_CASE_METHOD(mssql_fixture, "test_null", "[mssql][null]")
{
    test_null();
}

TEST_CASE_METHOD(mssql_fixture, "test_nullptr_nulls", "[mssql][null]")
{
    test_nullptr_nulls();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_iterator", "[mssql][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(mssql_fixture, "test_simple", "[mssql]")
{
    test_simple();
}

TEST_CASE_METHOD(mssql_fixture, "test_string", "[mssql][string]")
{
    test_string();
}

TEST_CASE_METHOD(mssql_fixture, "test_string_vector", "[mssql][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_binary", "[mssql][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(mssql_fixture, "test_time", "[mssql][time]")
{
    test_time();
}

TEST_CASE_METHOD(mssql_fixture, "test_date", "[mssql][date]")
{
    test_date();
}

TEST_CASE_METHOD(mssql_fixture, "test_datetime", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetime"), NANODBC_TEXT("d datetime"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    {
        execute(
            connection,
            NANODBC_TEXT("insert into test_datetime(d) values (CONVERT(datetime, "
                         "'2006-12-30T13:45:12.345', 126));"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetime;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_TYPE_TIMESTAMP);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetime"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour == 13);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_decimal", "[mssql][decimal]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_decimal"), NANODBC_TEXT("(d decimal(19,4))"));

    // insert
    {
        execute(
            connection,
            NANODBC_TEXT("insert into test_decimal(d) values (-922337203685477.5808);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal(d) values (0);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal(d) values (1.23);"));
        execute(
            connection, NANODBC_TEXT("insert into test_decimal(d) values (922337203685477.5807);"));
    }

    // select
    {
        auto result =
            execute(connection, NANODBC_TEXT("select d from test_decimal order by d asc;"));
        REQUIRE(result.next());
        auto d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("-922337203685477.5808")); // Min value of SQL data type
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT(".0000"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("1.2300"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("922337203685477.5807")); // Max value of SQL data type MONEY
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_money", "[mssql][decimal][money]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_money"), NANODBC_TEXT("(d money)"));

    // insert
    {
        execute(
            connection, NANODBC_TEXT("insert into test_money(d) values (-922337203685477.5808);"));
        execute(connection, NANODBC_TEXT("insert into test_money(d) values (0);"));
        execute(connection, NANODBC_TEXT("insert into test_money(d) values (1.23);"));
        execute(
            connection, NANODBC_TEXT("insert into test_money(d) values (922337203685477.5807);"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_money order by d asc;"));
        REQUIRE(result.next());
        auto d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("-922337203685477.5808")); // Min value of SQL data type MONEY
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT(".0000"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("1.2300"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("922337203685477.5807")); // Max value of SQL data type MONEY
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_datetime2", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetime2"), NANODBC_TEXT("d datetime2"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    {
        execute(
            connection,
            NANODBC_TEXT("insert into test_datetime2(d) values (CONVERT(datetime2, "
                         "'2006-12-30T13:45:12.345', 127));"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetime2;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_TYPE_TIMESTAMP);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetime2"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour == 13);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_datetimeoffset", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetimeoffset"), NANODBC_TEXT("d datetimeoffset"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    execute(
        connection,
        NANODBC_TEXT("insert into test_datetimeoffset(d) values "
                     "(CONVERT(datetimeoffset, '2006-12-30T13:45:12.345-08:00', 127));"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetimeoffset;"));

#ifndef SQL_SS_TIMESTAMPOFFSET
#define SQL_SS_TIMESTAMPOFFSET (-155)
#endif
        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_SS_TIMESTAMPOFFSET);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetimeoffset"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        // Currently, nanodbc binds SQL_SS_TIMESTAMPOFFSET as SQL_C_TIMESTAMP,
        // not as SQL_C_SS_TIMESTAMPOFFSET.
        // This seems to cause the DBMS or the driver to convert the time to local time
        // based on time zone
        // REQUIRE(t.hour == 13); // or 21 (GMT) or 22 (CET) or other client local time
        REQUIRE(t.hour > 0);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
        ;
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_transaction", "[mssql][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(mssql_fixture, "test_while_not_end_iteration", "[mssql][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(mssql_fixture, "test_while_next_iteration", "[mssql][looping]")
{
    test_while_next_iteration();
}

#if !defined(NANODBC_DISABLE_ASYNC) && defined(WIN32)
TEST_CASE_METHOD(mssql_fixture, "test_async", "[mssql][async]")
{
    HANDLE event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    nanodbc::connection conn;
    if (conn.async_connect(connection_string_, event_handle))
        WaitForSingleObject(event_handle, INFINITE);
    conn.async_complete();

    nanodbc::statement stmt(conn);
    if (stmt.async_prepare(NANODBC_TEXT("select count(*) from sys.tables;"), event_handle))
        WaitForSingleObject(event_handle, INFINITE);
    stmt.complete_prepare();

    if (stmt.async_execute(event_handle))
        WaitForSingleObject(event_handle, INFINITE);
    nanodbc::result row = stmt.complete_execute();

    if (row.async_next(event_handle))
        WaitForSingleObject(event_handle, INFINITE);
    REQUIRE(row.complete_next());

    REQUIRE(row.get<int>(0) >= 0);
}
#endif

#if defined(_MSC_VER) && defined(_UNICODE)
TEST_CASE_METHOD(mssql_fixture, "test_bind_variant", "[mssql][variant]")
{
    // Test prepared statement and binding Windows VARIANT data.
    auto conn = connect();
    create_table(
        conn,
        NANODBC_TEXT("test_bind_variant"),
        NANODBC_TEXT("(i int, f float, s varchar(256), d decimal(9, 3), b varbinary(max))"));

    nanodbc::statement stmt(conn);
    prepare(stmt, NANODBC_TEXT("insert into test_bind_variant(i,f,s,d,b) values (?,?,?,?,?)"));

    // NOTE: Some examples with number of round-trips below might seem redundant,
    // but it has been kept to for illustration purposes.

    // VT_I4 -> INT
    static_assert(sizeof(long) == sizeof(std::int32_t), "long is too large");
    _variant_t v_i(7L);
    stmt.bind(0, reinterpret_cast<std::int32_t*>(&v_i.lVal)); // no bind(long) provided
    // VT_R8 -> FLOAT
    _variant_t v_f(3.14);
    stmt.bind(1, &v_f.dblVal);
    // VT_BSTR -> VARCHAR
    _variant_t v_s(L"This is a text");
    stmt.bind_strings(2, reinterpret_cast<wchar_t*>(v_s.bstrVal), wcslen(v_s.bstrVal), 1);
    // VT_DECIMAL|VT_CY -> double -> DECIMAL(9,3)
    _variant_t v_d;
    {
        // use input value longer than DECIMAL(9,3) to test SQL will convert it appropriately
        DECIMAL d;
        VarDecFromStr(L"3.45612345", 0, LOCALE_NOUSEROVERRIDE, &d);
        double dbl;
        ::VarR8FromDec(&d, &dbl);
        v_d = dbl;
    }
    stmt.bind(3, &v_d.dblVal);
    // SAFEARRAY -> vector<uint8_t> -> VARBINARY
    // Since, currently, only way to bind binary data is via std::bector<std::uint8_t>,
    // we need to copy data from SAFEARRAY to intermediate vector.
    std::vector<std::uint8_t> bytes;
    {
        std::uint8_t data[] = {0x00, 0x01, 0x02, 0x03};
        CComSafeArray<std::uint8_t> sa;
        for (auto b : data)
            sa.Add(b);
        for (auto i = 0UL; i < sa.GetCount(); ++i)
            bytes.push_back(sa.GetAt(i));
    }
    std::vector<std::vector<std::uint8_t>> binary_items = {bytes};
    stmt.bind(4, binary_items);

    nanodbc::transact(stmt, 1);
    {
        auto result =
            nanodbc::execute(conn, NANODBC_TEXT("select i,f,s,d,b from test_bind_variant"));
        std::size_t i = 0;
        while (result.next())
        {
            REQUIRE(result.get<std::int32_t>(0) == static_cast<std::int32_t>(v_i));
            REQUIRE(result.get<double>(1) == static_cast<double>(v_f));
            REQUIRE(result.get<nanodbc::string>(2) == v_s.bstrVal);
            v_d.ChangeType(VT_BSTR);
            REQUIRE(result.get<nanodbc::string>(3) == nanodbc::string(v_d.bstrVal).substr(0, 5));
            REQUIRE(result.get<std::vector<std::uint8_t>>(4) == bytes);
            ++i;
        }
        REQUIRE(i == 1);
    }
}
#endif
