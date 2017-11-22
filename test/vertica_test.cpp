#include "test_case_fixture.h"

namespace
{
struct vertica_fixture : public test_case_fixture
{
    vertica_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_VERTICA");
    }
};
}

// TODO: add blob (bytea) test

TEST_CASE_METHOD(vertica_fixture, "test_driver", "[vertica][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(vertica_fixture, "test_batch_insert_integer", "[vertica][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(vertica_fixture, "test_batch_insert_string", "[vertica][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(vertica_fixture, "test_batch_insert_mixed", "[vertica][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(vertica_fixture, "test_catalog_list_catalogs", "[vertica][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(vertica_fixture, "test_catalog_list_schemas", "[vertica][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(vertica_fixture, "test_catalog_columns", "[vertica][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(vertica_fixture, "test_catalog_primary_keys", "[vertica][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(vertica_fixture, "test_catalog_tables", "[vertica][catalog][tables]")
{
    test_catalog_tables();
}

TEST_CASE_METHOD(vertica_fixture, "test_dbms_info", "[vertica][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(vertica_fixture, "test_get_info", "[vertica][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(vertica_fixture, "test_decimal_conversion", "[vertica][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(vertica_fixture, "test_exception", "[vertica][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    vertica_fixture,
    "test_execute_multiple_transaction",
    "[vertica][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(vertica_fixture, "test_execute_multiple", "[vertica][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(vertica_fixture, "test_integral", "[vertica][integral]")
{
    test_integral<vertica_fixture>();
}

TEST_CASE_METHOD(vertica_fixture, "test_move", "[vertica][move]")
{
    test_move();
}

TEST_CASE_METHOD(vertica_fixture, "test_null", "[vertica][null]")
{
    test_null();
}

TEST_CASE_METHOD(vertica_fixture, "test_result_iterator", "[vertica][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(vertica_fixture, "test_simple", "[vertica]")
{
    test_simple();
}

TEST_CASE_METHOD(vertica_fixture, "test_string", "[vertica][string]")
{
    test_string();
}

TEST_CASE_METHOD(vertica_fixture, "test_string_vector", "[vertica][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(vertica_fixture, "test_time", "[vertica][time]")
{
    test_time();
}

TEST_CASE_METHOD(vertica_fixture, "test_transaction", "[vertica][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(vertica_fixture, "test_batch_binary", "[vertica][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(vertica_fixture, "test_while_not_end_iteration", "[vertica][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(vertica_fixture, "test_while_next_iteration", "[vertica][looping]")
{
    test_while_next_iteration();
}
