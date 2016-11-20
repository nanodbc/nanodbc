#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
struct mssql_fixture : public base_test_fixture
{
    mssql_fixture() : base_test_fixture()
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
        REQUIRE(to_hex_string(blob) == "010100000000000000000059400000000000005940");
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

TEST_CASE_METHOD(
    mssql_fixture,
    "test_blob_with_varchar",
    "[mssql][blob][binary][varbinary][varchar]")
{
    nanodbc::string_type s = NANODBC_TEXT(
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
    REQUIRE(results.get<nanodbc::string_type>(0) == s);
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
    REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(
        results.get<nanodbc::string_type>(1) ==
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
    REQUIRE(
        results.get<nanodbc::string_type>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(
        results.get<nanodbc::string_type>(1) ==
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
    REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(
        results.get<nanodbc::string_type>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
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
