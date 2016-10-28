#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
struct mssql_fixture : public base_test_fixture
{
    mssql_fixture()
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        : base_test_fixture()
    {
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_MSSQL");
    }

    virtual ~mssql_fixture() NANODBC_NOEXCEPT {}
};
}

TEST_CASE_METHOD(mssql_fixture, "driver_test", "[mssql][driver]")
{
    driver_test();
}

TEST_CASE_METHOD(mssql_fixture, "affected_rows_test", "[mssql][affected_rows]")
{
    // Skip on SQL Server 2008, see details at
    // http://help.appveyor.com/discussions/problems/4704-database-cannot-be-autostarted-during-server-shutdown-or-startup
    if (get_env("DB") == NANODBC_TEXT("MSSQL2008"))
    {
        WARN("affected_rows_test skipped on AppVeyor with SQL Server 2008");
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

TEST_CASE_METHOD(mssql_fixture, "blob_test", "[mssql][blob][binary][varbinary]")
{
    nanodbc::connection connection = connect();
    // Test data size less than the default size of the internal buffer (1024)
    {
        create_table(connection, NANODBC_TEXT("blob_test"), NANODBC_TEXT("(data varbinary(max))"));
        execute(
            connection,
            NANODBC_TEXT("insert into blob_test values (CONVERT(varbinary(max), "
                         "'0x010100000000000000000059400000000000005940', 1));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from blob_test;"));
        REQUIRE(results.next());

        auto const blob = results.get<std::vector<std::uint8_t>>(0);
        REQUIRE(blob.size() == 21);
        REQUIRE(to_hex_string(blob) == "010100000000000000000059400000000000005940");
    }

    // Test data size greater than, but not multiple of, the default size of the internal buffer
    // (1024)
    {
        create_table(connection, NANODBC_TEXT("blob_test"), NANODBC_TEXT("(data varbinary(max))"));
        execute(connection, NANODBC_TEXT("insert into blob_test values (CRYPT_GEN_RANDOM(1579));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from blob_test;"));
        REQUIRE(results.next());
        REQUIRE(results.get<std::vector<std::uint8_t>>(0).size() == 1579);
    }
}

TEST_CASE_METHOD(
    mssql_fixture,
    "blob_test_with_varchar",
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
        connection, NANODBC_TEXT("blob_test_with_varchar"), NANODBC_TEXT("(data varbinary(max))"));
    execute(
        connection,
        NANODBC_TEXT("insert into blob_test_with_varchar values (CONVERT(varbinary(max), '") + s +
            NANODBC_TEXT("'));"));

    nanodbc::result results =
        nanodbc::execute(connection, NANODBC_TEXT("select data from blob_test_with_varchar;"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string_type>(0) == s);
}

TEST_CASE_METHOD(
    mssql_fixture,
    "block_cursor_with_nvarchar_test",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();

    // BLock Cursors: https://technet.microsoft.com/en-us/library/aa172590.aspx
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("variable_string_test"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into variable_string_test (i, s) values (1, 'this is a shorter text');"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into variable_string_test (i, s) values (2, 'this is a longer text of the two "
            "texts in the table');"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from variable_string_test order by i;"), rowset_size);
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
    "block_cursor_with_nvarchar_and_first_row_null_test",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("variable_string_test"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(conn, NANODBC_TEXT("insert into variable_string_test (i, s) values (1, NULL);"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into variable_string_test (i, s) values (2, 'this is a longer text of the two "
            "texts in the table');"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from variable_string_test order by i;"), rowset_size);
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
    "block_cursor_with_nvarchar_and_second_row_null_test",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("variable_string_test"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into variable_string_test (i, s) values (1, 'this is a shorter text');"));
    execute(conn, NANODBC_TEXT("insert into variable_string_test (i, s) values (2, NULL);"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from variable_string_test order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(results.get<nanodbc::string_type>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(
        results.get<nanodbc::string_type>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(mssql_fixture, "catalog_list_catalogs_test", "[mssql][catalog][catalogs]")
{
    catalog_list_catalogs_test();
}

TEST_CASE_METHOD(mssql_fixture, "catalog_list_schemas_test", "[mssql][catalog][schemas]")
{
    catalog_list_schemas_test();
}

TEST_CASE_METHOD(mssql_fixture, "catalog_columns_test", "[mssql][catalog][columns]")
{
    catalog_columns_test();
}

TEST_CASE_METHOD(mssql_fixture, "catalog_primary_keys_test", "[mssql][catalog][primary_keys]")
{
    catalog_primary_keys_test();
}

TEST_CASE_METHOD(mssql_fixture, "catalog_tables_test", "[mssql][catalog][tables]")
{
    catalog_tables_test();
}

TEST_CASE_METHOD(mssql_fixture, "catalog_table_privileges_test", "[mssql][catalog][tables]")
{
    catalog_table_privileges_test();
}

TEST_CASE_METHOD(mssql_fixture, "column_descriptor_test", "[mssql][columns]")
{
    column_descriptor_test();
}

TEST_CASE_METHOD(mssql_fixture, "dbms_info_test", "[mssql][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(mssql_fixture, "get_info_test", "[mssql][dmbs][metadata][info]")
{
    get_info_test();
}

TEST_CASE_METHOD(mssql_fixture, "decimal_conversion_test", "[mssql][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(mssql_fixture, "exception_test", "[mssql][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "execute_multiple_transaction_test",
    "[mssql][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(mssql_fixture, "execute_multiple_test", "[mssql][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(mssql_fixture, "integral_test", "[mssql][integral]")
{
    integral_test<mssql_fixture>();
}

TEST_CASE_METHOD(mssql_fixture, "move_test", "[mssql][move]")
{
    move_test();
}

TEST_CASE_METHOD(mssql_fixture, "null_test", "[mssql][null]")
{
    null_test();
}

TEST_CASE_METHOD(mssql_fixture, "nullptr_nulls_test", "[mssql][null]")
{
    nullptr_nulls_test();
}

TEST_CASE_METHOD(mssql_fixture, "result_iterator_test", "[mssql][iterator]")
{
    result_iterator_test();
}

TEST_CASE_METHOD(mssql_fixture, "simple_test", "[mssql]")
{
    simple_test();
}

TEST_CASE_METHOD(mssql_fixture, "string_test", "[mssql][string]")
{
    string_test();
}

TEST_CASE_METHOD(mssql_fixture, "string_vector_test", "[mssql][string]")
{
    string_vector_test();
}

TEST_CASE_METHOD(mssql_fixture, "transaction_test", "[mssql][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(mssql_fixture, "while_not_end_iteration_test", "[mssql][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(mssql_fixture, "while_next_iteration_test", "[mssql][looping]")
{
    while_next_iteration_test();
}

#if !defined(NANODBC_DISABLE_ASYNC) && defined(WIN32)
TEST_CASE_METHOD(mssql_fixture, "async_test", "[mssql][async]")
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
