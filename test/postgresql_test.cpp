#include "test_case_fixture.h"

#include <cstdio>
#include <cstdlib>

namespace
{
struct postgresql_fixture : public test_case_fixture
{
    postgresql_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_PGSQL");
    }
};
}

// TODO: add blob (bytea) test

TEST_CASE_METHOD(postgresql_fixture, "test_driver", "[postgresql][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(postgresql_fixture, "test_batch_insert_integer", "[postgresql][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(postgresql_fixture, "test_batch_insert_string", "[postgresql][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(postgresql_fixture, "test_batch_insert_mixed", "[postgresql][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_catalog_list_catalogs",
    "[postgresql][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(postgresql_fixture, "test_catalog_list_schemas", "[postgresql][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(postgresql_fixture, "test_catalog_columns", "[postgresql][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_catalog_primary_keys",
    "[postgresql][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(postgresql_fixture, "test_catalog_tables", "[postgresql][catalog][tables]")
{
    test_catalog_tables();
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_catalog_table_privileges",
    "[postgresql][catalog][tables]")
{
    test_catalog_table_privileges();
}

TEST_CASE_METHOD(postgresql_fixture, "test_column_descriptor", "[postgresql][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(postgresql_fixture, "test_dbms_info", "[postgresql][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(postgresql_fixture, "test_get_info", "[postgresql][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(postgresql_fixture, "test_decimal_conversion", "[postgresql][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(postgresql_fixture, "test_exception", "[postgresql][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_execute_multiple_transaction",
    "[postgresql][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(postgresql_fixture, "test_execute_multiple", "[postgresql][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(postgresql_fixture, "test_integral", "[postgresql][integral]")
{
    test_integral<postgresql_fixture>();
}

TEST_CASE_METHOD(postgresql_fixture, "test_move", "[postgresql][move]")
{
    test_move();
}

TEST_CASE_METHOD(postgresql_fixture, "test_null", "[postgresql][null]")
{
    test_null();
}

TEST_CASE_METHOD(postgresql_fixture, "test_result_iterator", "[postgresql][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(postgresql_fixture, "test_simple", "[postgresql]")
{
    test_simple();
}

TEST_CASE_METHOD(postgresql_fixture, "test_string", "[postgresql][string]")
{
    test_string();
}

TEST_CASE_METHOD(postgresql_fixture, "test_string_vector", "[postgresql][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(postgresql_fixture, "test_batch_binary", "[postgresql][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(postgresql_fixture, "test_time", "[postgresql][time]")
{
    test_time();
}

TEST_CASE_METHOD(postgresql_fixture, "test_time_without_time_zone", "[postgresql][time][timezone]")
{
    auto connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_time_without_tz"), NANODBC_TEXT("t time without time zone"));

    // insert
    execute(
        connection,
        NANODBC_TEXT("insert into test_time_without_tz(t) values "
                     "('2006-12-30 13:45:12.345'::time with time zone);"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select t from test_time_without_tz;"));
        REQUIRE(result.next());
        auto t = result.get<nanodbc::time>(0);
        REQUIRE(t.hour > 0);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
    }
}

TEST_CASE_METHOD(postgresql_fixture, "test_time_with_time_zone", "[postgresql][time][timezone]")
{
    auto connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_time_with_tz"), NANODBC_TEXT("t time with time zone"));

    // insert
    execute(
        connection,
        NANODBC_TEXT("insert into test_time_with_tz(t) values "
                     "('2006-12-30 13:45:12.345-08:00'::time with time zone);"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select t from test_time_with_tz;"));
        REQUIRE(result.next());
        // pgsqlODBC reports 'time with time zone' as SQL_WVARCHAR, not SQL_TIME.
        // See https://github.com/lexicalunit/nanodbc/pull/229
        auto t = result.get<nanodbc::string>(0);
        REQUIRE(t == NANODBC_TEXT("13:45:12.345-08"));
    }
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_timestamp_without_time_zone",
    "[postgresql][timestamp][timezone]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_timestamp_without_tz"),
        NANODBC_TEXT("t timestamp without time zone"));

    // insert
    execute(
        connection,
        NANODBC_TEXT("insert into test_timestamp_without_tz(t) values "
                     "('2006-12-30 13:45:12.345'::timestamp with time zone);"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select t from test_timestamp_without_tz;"));
        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour > 0);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(
    postgresql_fixture,
    "test_timestamp_with_time_zone",
    "[postgresql][timestamp][timezone]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_timestamp_with_tz"),
        NANODBC_TEXT("t timestamp with time zone"));

    // insert
    execute(
        connection,
        NANODBC_TEXT("insert into test_timestamp_with_tz(t) values "
                     "('2006-12-30 13:45:12.345-08:00'::timestamp with time zone);"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select t from test_timestamp_with_tz;"));
        REQUIRE(result.next());
        // Note, unlike 'time with time zone' reported as SQL_WVARCHAR, but not SQL_TIME,
        // 'timestamp with time zone' seems reported as SQL_TIMESTAMP.
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour > 0);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(postgresql_fixture, "test_transaction", "[postgresql][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(postgresql_fixture, "test_while_not_end_iteration", "[postgresql][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(postgresql_fixture, "test_while_next_iteration", "[postgresql][looping]")
{
    test_while_next_iteration();
}
