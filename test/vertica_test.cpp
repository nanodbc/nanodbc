#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
struct vertica_fixture : public base_test_fixture
{
    vertica_fixture()
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        : base_test_fixture()
    {
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_VERTICA");
    }

    virtual ~vertica_fixture() NANODBC_NOEXCEPT {}
};
}

// TODO: add blob (bytea) test

TEST_CASE_METHOD(vertica_fixture, "driver_test", "[vertica][driver]")
{
    driver_test();
}

TEST_CASE_METHOD(vertica_fixture, "catalog_list_catalogs_test", "[vertica][catalog][catalogs]")
{
    catalog_list_catalogs_test();
}

TEST_CASE_METHOD(vertica_fixture, "catalog_list_schemas_test", "[vertica][catalog][schemas]")
{
    catalog_list_schemas_test();
}

TEST_CASE_METHOD(vertica_fixture, "catalog_columns_test", "[vertica][catalog][columns]")
{
    catalog_columns_test();
}

TEST_CASE_METHOD(vertica_fixture, "catalog_primary_keys_test", "[vertica][catalog][primary_keys]")
{
    catalog_primary_keys_test();
}

TEST_CASE_METHOD(vertica_fixture, "catalog_tables_test", "[vertica][catalog][tables]")
{
    catalog_tables_test();
}

TEST_CASE_METHOD(vertica_fixture, "dbms_info_test", "[vertica][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(vertica_fixture, "get_info_test", "[vertica][dmbs][metadata][info]")
{
    get_info_test();
}

TEST_CASE_METHOD(vertica_fixture, "decimal_conversion_test", "[vertica][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(vertica_fixture, "exception_test", "[vertica][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(
    vertica_fixture,
    "execute_multiple_transaction_test",
    "[vertica][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(vertica_fixture, "execute_multiple_test", "[vertica][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(vertica_fixture, "integral_test", "[vertica][integral]")
{
    integral_test<vertica_fixture>();
}

TEST_CASE_METHOD(vertica_fixture, "move_test", "[vertica][move]")
{
    move_test();
}

TEST_CASE_METHOD(vertica_fixture, "null_test", "[vertica][null]")
{
    null_test();
}

TEST_CASE_METHOD(vertica_fixture, "result_iterator_test", "[vertica][iterator]")
{
    result_iterator_test();
}

TEST_CASE_METHOD(vertica_fixture, "simple_test", "[vertica]")
{
    simple_test();
}

TEST_CASE_METHOD(vertica_fixture, "string_test", "[vertica][string]")
{
    string_test();
}

TEST_CASE_METHOD(vertica_fixture, "string_vector_test", "[vertica][string]")
{
    string_vector_test();
}

TEST_CASE_METHOD(vertica_fixture, "transaction_test", "[vertica][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(vertica_fixture, "while_not_end_iteration_test", "[vertica][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(vertica_fixture, "while_next_iteration_test", "[vertica][looping]")
{
    while_next_iteration_test();
}
