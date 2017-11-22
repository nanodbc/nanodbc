#include "test_case_fixture.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
struct mysql_fixture : public test_case_fixture
{
    mysql_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_MYSQL");
    }
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
    // Inseting/retrieving long strings
    {
        nanodbc::string long_string(1024, '\0');
        for (unsigned i = 0; i < 1024; i++)
            long_string[i] = (i % 64) + 32;

        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_longstring (t TEXT NOT NULL)"));
        REQUIRE(result.affected_rows() == 0);

        nanodbc::statement stmt(conn, NANODBC_TEXT("INSERT INTO nanodbc_longstring VALUES (?)"));
        stmt.bind(0, long_string.c_str());
        result = stmt.execute();
        REQUIRE(result.affected_rows() == 1);

        result = execute(conn, NANODBC_TEXT("SELECT t FROM nanodbc_longstring LIMIT 1"));
        REQUIRE(result.affected_rows() == 1);

        if (result.next())
        {
            nanodbc::string str_from_db = result.get<nanodbc::string>(0);
            REQUIRE(str_from_db == long_string);
        }
    }
    // DROP DATABASE|TABLE
    {
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("DROP TABLE nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 0);
        result = execute(conn, NANODBC_TEXT("DROP TABLE nanodbc_longstring"));
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

TEST_CASE_METHOD(
    mysql_fixture,
    "test_execute_multiple_transaction",
    "[mysql][execute][transaction]")
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

TEST_CASE_METHOD(mysql_fixture, "test_batch_binary", "[mysql][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(mysql_fixture, "test_time", "[mysql][time]")
{
    test_time();
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
