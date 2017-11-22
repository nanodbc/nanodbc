#include "test_case_fixture.h"

namespace
{

struct sqlite_fixture : public test_case_fixture
{
    sqlite_fixture()
        : test_case_fixture()
    {
        // According to the sqliteodbc documentation,
        // driver name is different on Windows and Unix.
        nanodbc::string const driver_name =
#ifdef _WIN32
            (NANODBC_TEXT("SQLite3 ODBC Driver"));
#else
            (NANODBC_TEXT("SQLite3"));
#endif

        connection_string_ =
            NANODBC_TEXT("Driver=") + driver_name + NANODBC_TEXT(";Database=nanodbc.db;");

        sqlite_cleanup(); // in case prior test exited without proper cleanup
    }

    virtual ~sqlite_fixture() NANODBC_NOEXCEPT { sqlite_cleanup(); }

    void sqlite_cleanup() NANODBC_NOEXCEPT
    {
        int success = std::remove("nanodbc.db");
        (void)success;
    }

    void before_catalog_test()
    {
        // Since SQLite does not have metadata tables,
        // we need to ensure, there is at least one table to search for.
        auto conn = connect();
        create_table(
            conn, NANODBC_TEXT("catalog_tables_test"), NANODBC_TEXT("(a int PRIMARY KEY, b text)"));
    }
};
}

// Unicode build on Ubuntu 12.04 with unixODBC 2.2.14p2 and libsqliteodbc 0.91-3 throws:
// test/sqlite_test.cpp:42: FAILED:
// due to a fatal error condition:
//   SIGSEGV - Segmentation violation signal
// See discussions at
// https://github.com/lexicalunit/nanodbc/pull/154
// https://groups.google.com/forum/#!msg/catch-forum/7tIpgm8SvDA/1QZZESIuCQAJ
// TODO: Uncomment as soon as the SIGSEGV issue has been fixed.
#ifndef NANODBC_ENABLE_UNICODE
TEST_CASE_METHOD(sqlite_fixture, "test_affected_rows", "[sqlite][affected_rows]")
{
    nanodbc::connection conn = connect();

    // CREATE TABLE  (CREATE DATABASE not supported)
    {
        auto result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
        REQUIRE(result.affected_rows() == 0);
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
        // auto result = execute(conn, NANODBC_TEXT("SELECT i FROM nanodbc_test_temp_table"));
        // REQUIRE(result.affected_rows() == 0);
    } // DELETE
    {
        auto result = execute(conn, NANODBC_TEXT("DELETE FROM nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 2);
        // then DROP TABLE
        {
            auto result2 = execute(conn, NANODBC_TEXT("DROP TABLE nanodbc_test_temp_table"));
            REQUIRE(result2.affected_rows() == 2);
        }
    }

    // DROP TABLE, without prior DELETE
    {
        execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
        execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (1)"));
        execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (2)"));

        auto result = execute(conn, NANODBC_TEXT("DROP TABLE nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 1);
    }
}
#endif

TEST_CASE_METHOD(sqlite_fixture, "test_driver", "[sqlite][driver]")
{
    test_driver();
}

// TODO: Investigate why these tests fail on Linux
// See https://github.com/lexicalunit/nanodbc/pull/220#issuecomment-257029475
#ifdef _WIN32
TEST_CASE_METHOD(sqlite_fixture, "test_batch_insert_integral", "[sqlite][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(sqlite_fixture, "test_batch_insert_mixed", "[sqlite][batch]")
{
    test_batch_insert_mixed();
}
#endif // _WIN32

TEST_CASE_METHOD(sqlite_fixture, "test_batch_insert_string", "[sqlite][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(sqlite_fixture, "test_blob", "[sqlite][blob]")
{
    test_blob();
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_list_catalogs", "[sqlite][catalog][catalogs]")
{
    before_catalog_test();

    auto conn = connect();
    REQUIRE(conn.connected());
    nanodbc::catalog catalog(conn);

    auto names = catalog.list_catalogs();
    // NOTE: SQLite ODBC behaviour tested below has been revealed with run-time experiments,
    //       and no documentation to confirm it has been found.
    REQUIRE(names.size() == 1);
    REQUIRE(names.front().empty());
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_list_schemas", "[sqlite][catalog][schemas]")
{
    before_catalog_test();

    auto conn = connect();
    REQUIRE(conn.connected());
    nanodbc::catalog catalog(conn);

    auto names = catalog.list_catalogs();
    // NOTE: SQLite ODBC behaviour tested below has been revealed with run-time experiments,
    //       and no documentation to confirm it has been found.
    REQUIRE(names.size() == 1);
    REQUIRE(names.front().empty());
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_columns", "[sqlite][catalog][columns]")
{
    before_catalog_test();
    test_catalog_columns();
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_primary_keys", "[sqlite][catalog][primary_keys]")
{
    before_catalog_test();
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_tables", "[sqlite][catalog][tables]")
{
    before_catalog_test();
    test_catalog_tables();
}

TEST_CASE_METHOD(sqlite_fixture, "test_catalog_table_privileges", "[sqlite][catalog][tables]")
{
    before_catalog_test();
    test_catalog_table_privileges();
}

TEST_CASE_METHOD(sqlite_fixture, "test_column_descriptor", "[sqlite][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(sqlite_fixture, "test_date", "[sqlite][date]")
{
    test_date();
}

TEST_CASE_METHOD(sqlite_fixture, "test_dbms_info", "[sqlite][dmbs][metadata][info]")
{
    test_dbms_info();

    // Additional SQLite-specific checks
    nanodbc::connection connection = connect();
    REQUIRE(connection.dbms_name() == NANODBC_TEXT("SQLite"));
}

TEST_CASE_METHOD(sqlite_fixture, "test_get_info", "[sqlite][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(sqlite_fixture, "test_decimal_conversion", "[sqlite][decimal][conversion]")
{
    // SQLite ODBC driver requires dedicated test.
    // The driver converts SQL DECIMAL value to C float value
    // without preserving DECIMAL(N,M) dimensions
    // and strips any trailing zeros.
    nanodbc::connection connection = connect();
    nanodbc::result results;

    create_table(
        connection, NANODBC_TEXT("test_decimal_conversion"), NANODBC_TEXT("(d decimal(9, 3))"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (12345.987);"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (5.6);"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (1);"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (-1.333);"));
    results =
        execute(connection, NANODBC_TEXT("select * from test_decimal_conversion order by 1 desc;"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("12345.987"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("5.6"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("1"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("-1.333"));
}

TEST_CASE_METHOD(sqlite_fixture, "test_exception", "[sqlite][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    sqlite_fixture,
    "test_execute_multiple_transaction",
    "[sqlite][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(sqlite_fixture, "test_execute_multiple", "[sqlite][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(sqlite_fixture, "test_integral", "[sqlite][integral]")
{
    test_integral<sqlite_fixture>();
}

TEST_CASE_METHOD(sqlite_fixture, "test_integral_boundary", "[sqlite][integral]")
{
    nanodbc::connection connection = connect();
    drop_table(connection, NANODBC_TEXT("test_integral_boundary"));

    // SQLite3 uses single storage class INTEGER for all integral SQL types
    execute(
        connection,
        NANODBC_TEXT(
            "create table test_integral_boundary(i1 integer,i2 integer,i4 integer,i8 integer);"));

    auto const sql =
        NANODBC_TEXT("insert into test_integral_boundary(i1,i2,i4,i8) values (?,?,?,?);");

    std::int16_t const i1min = std::numeric_limits<std::int8_t>::min();
    auto const i2min = std::numeric_limits<std::int16_t>::min();
    auto const i4min = std::numeric_limits<std::int32_t>::min();
    auto const i8min = std::numeric_limits<std::int64_t>::min();
    std::int16_t const i1max = std::numeric_limits<std::int8_t>::max();
    auto const i2max = std::numeric_limits<std::int16_t>::max();
    auto const i4max = std::numeric_limits<std::int32_t>::max();
    auto const i8max = std::numeric_limits<std::int64_t>::max();

    // min
    {
        nanodbc::statement statement(connection);
        prepare(statement, sql);
        statement.bind(0, &i1min);
        statement.bind(1, &i2min);
        statement.bind(2, &i4min);
        statement.bind(3, &i8min);
        REQUIRE(statement.connected());
        execute(statement);
    }

    // max
    {
        nanodbc::statement statement(connection);
        prepare(statement, sql);
        statement.bind(0, &i1max);
        statement.bind(1, &i2max);
        statement.bind(2, &i4max);
        statement.bind(3, &i8max);
        REQUIRE(statement.connected());
        execute(statement);
    }

    // query
    nanodbc::result result = execute(
        connection,
        NANODBC_TEXT("select i1,i2,i4,i8 from test_integral_boundary order by i1 asc;"));
    // min
    REQUIRE(result.next());
    // All of string converted values are incorrect
    // auto si1min = result.get<nanodbc::string>(0);
    // auto si2min = result.get<nanodbc::string>(1);
    // auto si4min = result.get<nanodbc::string>(2);
    // auto si8min = result.get<nanodbc::string>(3);
    REQUIRE(result.get<std::int16_t>(0) == static_cast<std::int16_t>(i1min));
    REQUIRE(result.get<std::int16_t>(1) == i2min);
    REQUIRE(result.get<std::int32_t>(2) == i4min);
    REQUIRE(result.get<std::int64_t>(3) == i8min);
    // max
    REQUIRE(result.next());
    REQUIRE(result.get<std::int16_t>(0) == static_cast<std::int16_t>(i1max));
    REQUIRE(result.get<std::int16_t>(1) == i2max);
    REQUIRE(result.get<std::int32_t>(2) == i4max);
    REQUIRE(result.get<std::int64_t>(3) == i8max);
    // query end
    REQUIRE(!result.next());
}

TEST_CASE_METHOD(sqlite_fixture, "test_integral_to_string_conversion", "[sqlite][integral]")
{
    // TODO: Move to common tests
    nanodbc::connection connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_integral_to_string_conversion"),
        NANODBC_TEXT("(i int, n int)"));
    execute(
        connection, NANODBC_TEXT("insert into test_integral_to_string_conversion values (1, 0);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (2, 255);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (3, -128);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (4, 127);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (5, -32768);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (6, 32767);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (7, -2147483648);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_integral_to_string_conversion values (8, 2147483647);"));
    auto results = execute(
        connection,
        NANODBC_TEXT("select * from test_integral_to_string_conversion order by i asc;"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("0"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("255"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("-128"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("127"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("-32768"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("32767"));
    REQUIRE(results.next());
    // FIXME: SQLite ODBC driver reports column size of 10 leading to truncation
    // "-214748364" == "-2147483648"
    // The driver bug?
    // REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("-2147483648"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("2147483647"));
}

TEST_CASE_METHOD(sqlite_fixture, "test_move", "[sqlite][move]")
{
    test_move();
}

TEST_CASE_METHOD(sqlite_fixture, "test_null", "[sqlite][null]")
{
    test_null();
}

TEST_CASE_METHOD(sqlite_fixture, "test_nullptr_nulls", "[sqlite][null]")
{
    test_nullptr_nulls();
}

TEST_CASE_METHOD(sqlite_fixture, "test_result_iterator", "[sqlite][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(sqlite_fixture, "test_simple", "[sqlite]")
{
    test_simple();
}

TEST_CASE_METHOD(sqlite_fixture, "test_string", "[sqlite][string]")
{
    test_string();
}

TEST_CASE_METHOD(sqlite_fixture, "test_string_vector", "[sqlite][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(sqlite_fixture, "test_batch_binary", "[sqlite][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(sqlite_fixture, "test_time", "[sqlite][time]")
{
    test_time();
}

TEST_CASE_METHOD(sqlite_fixture, "test_transaction", "[sqlite][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(sqlite_fixture, "test_while_not_end_iteration", "[sqlite][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(sqlite_fixture, "test_while_next_iteration", "[sqlite][looping]")
{
    test_while_next_iteration();
}
