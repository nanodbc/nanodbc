#include "test_case_fixture.h"

namespace
{
struct db2_fixture : public test_case_fixture
{
    db2_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_DB2");
    }
};
} // namespace

TEST_CASE_METHOD(db2_fixture, "test_driver", "[db2][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(db2_fixture, "test_driver_info", "[db2][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(db2_fixture, "test_datasources", "[db2][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(db2_fixture, "test_batch_insert_integer", "[db2][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(db2_fixture, "test_batch_insert_null", "[db2][batch][null]")
{
    test_batch_insert_null();
}

TEST_CASE_METHOD(db2_fixture, "test_batch_insert_string", "[db2][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(db2_fixture, "test_batch_insert_mixed", "[db2][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(db2_fixture, "test_catalog_list_catalogs", "[db2][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(db2_fixture, "test_catalog_list_schemas", "[db2][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(
    db2_fixture,
    "test_catalog_list_table_types",
    "[db2][catalog][table_types]")
{
    test_catalog_list_table_types();
}

TEST_CASE_METHOD(db2_fixture, "test_catalog_columns", "[db2][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(db2_fixture, "test_catalog_primary_keys", "[db2][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(db2_fixture, "test_catalog_tables", "[db2][catalog][tables]")
{
    test_catalog_tables();
}

TEST_CASE_METHOD(db2_fixture, "test_connection_environment", "[db2][connection]")
{
    test_connection_environment();
}

TEST_CASE_METHOD(db2_fixture, "test_dbms_info", "[db2][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(db2_fixture, "test_get_info", "[db2][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(db2_fixture, "test_decimal_conversion", "[db2][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(db2_fixture, "test_error", "[db2][error]")
{
    test_error();
}

TEST_CASE_METHOD(db2_fixture, "test_exception", "[db2][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    db2_fixture,
    "test_execute_multiple_transaction",
    "[db2][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(db2_fixture, "test_execute_multiple", "[db2][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(db2_fixture, "test_integral", "[db2][integral]")
{
    test_integral<db2_fixture>();
}

TEST_CASE_METHOD(db2_fixture, "test_integral_small_types", "[db2][integral]")
{
    test_integral_small_types();
}

TEST_CASE_METHOD(db2_fixture, "test_integral_small_types_batch", "[db2][integral][batch]")
{
    test_integral_small_types_batch();
}

TEST_CASE_METHOD(db2_fixture, "test_move", "[db2][move]")
{
    test_move();
}

TEST_CASE_METHOD(db2_fixture, "test_null", "[db2][null]")
{
    test_null();
}

TEST_CASE_METHOD(
    db2_fixture,
    "test_null_with_bound_columns_unbound",
    "[db2][null][unbound]")
{
    test_null_with_bound_columns_unbound();
}

TEST_CASE_METHOD(db2_fixture, "test_result_at_end", "[db2][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(db2_fixture, "test_result_iterator", "[db2][result][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(db2_fixture, "test_simple", "[db2]")
{
    test_simple();
}

TEST_CASE_METHOD(db2_fixture, "test_statement_usable_when_result_gone", "[db2][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(db2_fixture, "test_string", "[db2][string]")
{
    test_string();
}

TEST_CASE_METHOD(db2_fixture, "test_string_vector", "[db2][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(db2_fixture, "test_string_view_vector", "[db2][string]")
{
    test_string_view_vector();
}

TEST_CASE_METHOD(db2_fixture, "test_time", "[db2][time]")
{
    test_time();
}

TEST_CASE_METHOD(db2_fixture, "test_transaction", "[db2][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(db2_fixture, "test_blob_binary", "[db2][blob][binary]")
{
    test_blob_binary();
}

TEST_CASE_METHOD(db2_fixture, "test_batch_binary", "[db2][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(db2_fixture, "test_while_not_end_iteration", "[db2][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(db2_fixture, "test_while_next_iteration", "[db2][looping]")
{
    test_while_next_iteration();
}
