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

TEST_CASE_METHOD(mysql_fixture, "test_driver", "[mysql][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(mysql_fixture, "test_affected_rows", "[mysql][affected_rows]")
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

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_integer", "[mysql][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_string", "[mysql][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_mixed", "[mysql][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(mysql_fixture, "test_blob", "[mysql][blob]")
{
    test_blob();
}

TEST_CASE_METHOD(mysql_fixture, "test_catalog_list_catalogs", "[mysql][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(mysql_fixture, "test_catalog_list_schemas", "[mysql][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(mysql_fixture, "test_catalog_columns", "[mysql][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(mysql_fixture, "test_catalog_primary_keys", "[mysql][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(mysql_fixture, "test_catalog_tables", "[mysql][catalog][tables]")
{
    test_catalog_tables();
}

// TODO: Add test_catalog_table_privileges - SQLTablePrivileges returns empty result set

TEST_CASE_METHOD(mysql_fixture, "test_column_descriptor", "[mysql][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(mysql_fixture, "test_dbms_info", "[mysql][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(mysql_fixture, "test_get_info", "[mysql][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(mysql_fixture, "test_decimal_conversion", "[mysql][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(mysql_fixture, "test_exception", "[mysql][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(mysql_fixture, "test_execute_multiple_transaction", "[mysql][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(mysql_fixture, "test_execute_multiple", "[mysql][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(mysql_fixture, "test_integral", "[mysql][integral]")
{
    test_integral<mysql_fixture>();
}

TEST_CASE_METHOD(mysql_fixture, "test_move", "[mysql][move]")
{
    test_move();
}

TEST_CASE_METHOD(mysql_fixture, "test_null", "[mysql][null]")
{
    test_null();
}

TEST_CASE_METHOD(mysql_fixture, "test_nullptr_nulls", "[mysql][null]")
{
    test_nullptr_nulls();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_iterator", "[mysql][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(mysql_fixture, "test_simple", "[mysql]")
{
    test_simple();
}

TEST_CASE_METHOD(mysql_fixture, "test_string", "[mysql][string]")
{
    test_string();
}

TEST_CASE_METHOD(mysql_fixture, "test_string_vector", "[mysql][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(mysql_fixture, "batch_binary_test", "[mysql][binary]")
{
    batch_binary_test();
}

TEST_CASE_METHOD(mysql_fixture, "test_transaction", "[mysql][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(mysql_fixture, "test_while_not_end_iteration", "[mysql][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(mysql_fixture, "test_while_next_iteration", "[mysql][looping]")
{
    test_while_next_iteration();
}
