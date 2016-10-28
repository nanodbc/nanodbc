#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
struct odbc_fixture : public base_test_fixture
{
    odbc_fixture()
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        : base_test_fixture()
    {
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_ODBC");
    }

    virtual ~odbc_fixture() NANODBC_NOEXCEPT {}
};
}

TEST_CASE_METHOD(odbc_fixture, "driver_test", "[odbc][driver]")
{
    driver_test();
}

TEST_CASE_METHOD(odbc_fixture, "blob_test", "[odbc][blob]")
{
    blob_test();
}

TEST_CASE_METHOD(odbc_fixture, "catalog_list_catalogs_test", "[odbc][catalog][catalogs]")
{
    catalog_list_catalogs_test();
}

TEST_CASE_METHOD(odbc_fixture, "catalog_list_schemas_test", "[odbc][catalog][schemas]")
{
    catalog_list_schemas_test();
}

TEST_CASE_METHOD(odbc_fixture, "catalog_columns_test", "[odbc][catalog][columns]")
{
    catalog_columns_test();
}

TEST_CASE_METHOD(odbc_fixture, "catalog_primary_keys_test", "[odbc][catalog][primary_keys]")
{
    catalog_primary_keys_test();
}

TEST_CASE_METHOD(odbc_fixture, "catalog_tables_test", "[odbc][catalog][tables]")
{
    catalog_tables_test();
}

TEST_CASE_METHOD(odbc_fixture, "dbms_info_test", "[odbc][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(odbc_fixture, "get_info_test", "[odbc][dmbs][metadata][info]")
{
    get_info_test();
}

TEST_CASE_METHOD(odbc_fixture, "decimal_conversion_test", "[odbc][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(odbc_fixture, "exception_test", "[odbc][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(odbc_fixture, "execute_multiple_transaction_test", "[odbc][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(odbc_fixture, "execute_multiple_test", "[odbc][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(odbc_fixture, "integral_test", "[odbc][integral]")
{
    integral_test<odbc_fixture>();
}

TEST_CASE_METHOD(odbc_fixture, "move_test", "[odbc][move]")
{
    move_test();
}

TEST_CASE_METHOD(odbc_fixture, "null_test", "[odbc][null]")
{
    null_test();
}

TEST_CASE_METHOD(odbc_fixture, "nullptr_nulls_test", "[odbc][null]")
{
    nullptr_nulls_test();
}

TEST_CASE_METHOD(odbc_fixture, "result_iterator_test", "[odbc][iterator]")
{
    result_iterator_test();
}

TEST_CASE_METHOD(odbc_fixture, "simple_test", "[odbc]")
{
    simple_test();
}

TEST_CASE_METHOD(odbc_fixture, "string_test", "[odbc][string]")
{
    string_test();
}

TEST_CASE_METHOD(odbc_fixture, "string_vector_test", "[odbc][string]")
{
    string_vector_test();
}

TEST_CASE_METHOD(odbc_fixture, "transaction_test", "[odbc][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(odbc_fixture, "while_not_end_iteration_test", "[odbc][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(odbc_fixture, "while_next_iteration_test", "[odbc][looping]")
{
    while_next_iteration_test();
}
