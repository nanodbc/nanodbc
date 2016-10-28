#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
struct mysql_fixture : public base_test_fixture
{
    mysql_fixture()
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        : base_test_fixture()
    {
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_MYSQL");
    }

    virtual ~mysql_fixture() NANODBC_NOEXCEPT {}
};
}

// FIXME: No catlog_* tests for MySQL. Not supported?

TEST_CASE_METHOD(mysql_fixture, "driver_test", "[mysql][driver]")
{
    driver_test();
}

TEST_CASE_METHOD(mysql_fixture, "affected_rows_test", "[mysql][affected_rows]")
{
    nanodbc::connection conn = connect();

    // CREATE DATABASE|TABLE
    {
        execute(conn, NANODBC_TEXT("DROP DATABASE IF EXISTS nanodbc_test_temp_db"));
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("CREATE DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.affected_rows() == 1);
        execute(conn, NANODBC_TEXT("USE nanodbc_test_temp_db"));
        result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
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
        auto result = execute(conn, NANODBC_TEXT("SELECT i FROM nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 2);
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
        REQUIRE(result.affected_rows() == 0);
        result = execute(conn, NANODBC_TEXT("DROP DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.affected_rows() == 0);
    }
}

TEST_CASE_METHOD(mysql_fixture, "blob_test", "[mysql][blob]")
{
    blob_test();
}

TEST_CASE_METHOD(mysql_fixture, "catalog_list_catalogs_test", "[mysql][catalog][catalogs]")
{
    catalog_list_catalogs_test();
}

TEST_CASE_METHOD(mysql_fixture, "catalog_list_schemas_test", "[mysql][catalog][schemas]")
{
    catalog_list_schemas_test();
}

TEST_CASE_METHOD(mysql_fixture, "catalog_columns_test", "[mysql][catalog][columns]")
{
    catalog_columns_test();
}

TEST_CASE_METHOD(mysql_fixture, "catalog_primary_keys_test", "[mysql][catalog][primary_keys]")
{
    catalog_primary_keys_test();
}

TEST_CASE_METHOD(mysql_fixture, "catalog_tables_test", "[mysql][catalog][tables]")
{
    catalog_tables_test();
}

// TODO: Add catalog_table_privileges_test - SQLTablePrivileges returns empty result set

TEST_CASE_METHOD(mysql_fixture, "column_descriptor_test", "[mysql][columns]")
{
    column_descriptor_test();
}

TEST_CASE_METHOD(mysql_fixture, "dbms_info_test", "[mysql][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(mysql_fixture, "get_info_test", "[mysql][dmbs][metadata][info]")
{
    get_info_test();
}

TEST_CASE_METHOD(mysql_fixture, "decimal_conversion_test", "[mysql][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(mysql_fixture, "exception_test", "[mysql][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "execute_multiple_transaction_test",
    "[mysql][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(mysql_fixture, "execute_multiple_test", "[mysql][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(mysql_fixture, "integral_test", "[mysql][integral]")
{
    integral_test<mysql_fixture>();
}

TEST_CASE_METHOD(mysql_fixture, "move_test", "[mysql][move]")
{
    move_test();
}

TEST_CASE_METHOD(mysql_fixture, "null_test", "[mysql][null]")
{
    null_test();
}

TEST_CASE_METHOD(mysql_fixture, "nullptr_nulls_test", "[mysql][null]")
{
    nullptr_nulls_test();
}

TEST_CASE_METHOD(mysql_fixture, "result_iterator_test", "[mysql][iterator]")
{
    result_iterator_test();
}

TEST_CASE_METHOD(mysql_fixture, "simple_test", "[mysql]")
{
    simple_test();
}

TEST_CASE_METHOD(mysql_fixture, "string_test", "[mysql][string]")
{
    string_test();
}

TEST_CASE_METHOD(mysql_fixture, "string_vector_test", "[mysql][string]")
{
    string_vector_test();
}

TEST_CASE_METHOD(mysql_fixture, "transaction_test", "[mysql][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(mysql_fixture, "while_not_end_iteration_test", "[mysql][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(mysql_fixture, "while_next_iteration_test", "[mysql][looping]")
{
    while_next_iteration_test();
}
