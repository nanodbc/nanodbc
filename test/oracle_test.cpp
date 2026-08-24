#include "test_case_fixture.h"

namespace
{
struct oracle_fixture : public test_case_fixture
{
    oracle_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_ORACLE");
    }
};
} // namespace

// Oracle has no TIME type, only DATE and TIMESTAMP, so test_time has nothing to create a
// column with here and is left out rather than made to pass against a different type.
// Oracle's DATE carries a time of day, which the driver reports as SQL_TYPE_TIMESTAMP, so
// test_date is left out on the same grounds.

TEST_CASE_METHOD(oracle_fixture, "test_batch_binary", "[oracle][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(oracle_fixture, "test_batch_delete", "[oracle][batch][delete]")
{
    test_batch_delete();
}

TEST_CASE_METHOD(oracle_fixture, "test_batch_insert_mixed", "[oracle][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_rows", "[oracle][batch][bind_rows]")
{
    test_bind_rows();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_rows_null", "[oracle][batch][bind_rows]")
{
    test_bind_rows_null();
}

TEST_CASE_METHOD(oracle_fixture, "test_batch_insert_null", "[oracle][batch][null]")
{
    test_batch_insert_null();
}

TEST_CASE_METHOD(oracle_fixture, "test_batch_insert_string", "[oracle][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(oracle_fixture, "test_binary_read_shapes", "[oracle][result][binary]")
{
    test_binary_read_shapes();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_arithmetic_null_sentry", "[oracle][bind][null]")
{
    test_bind_arithmetic_null_sentry();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_binary_null_sentry", "[oracle][binary][null]")
{
    test_bind_binary_null_sentry();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_date_to_timestamp_parameter", "[oracle][bind][date]")
{
    test_bind_date_to_timestamp_parameter();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_every_form", "[oracle][bind][batch]")
{
    test_bind_every_form();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_null_array", "[oracle][null]")
{
    test_bind_null_array();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_null_in_single_row_batch", "[oracle][bind][null]")
{
    test_bind_null_in_single_row_batch();
}

TEST_CASE_METHOD(oracle_fixture, "test_bind_null_sentry", "[oracle][statement]")
{
    test_bind_null_sentry();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_bind_timestamp_as_string",
    "[oracle][statement][bind][timestamp]")
{
    test_bind_timestamp_as_string();
}

TEST_CASE_METHOD(oracle_fixture, "test_column_descriptor", "[oracle][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(oracle_fixture, "test_connection_catalog_name", "[oracle][connection][metadata]")
{
    test_connection_catalog_name();
}

TEST_CASE_METHOD(oracle_fixture, "test_connection_environment", "[oracle][connection]")
{
    test_connection_environment();
}

TEST_CASE_METHOD(oracle_fixture, "test_connection_attributes", "[oracle][connection][attributes]")
{
    test_connection_attributes();
}

TEST_CASE_METHOD(oracle_fixture, "test_connection_per_thread", "[oracle][connection][threads]")
{
    test_connection_per_thread();
}

TEST_CASE_METHOD(oracle_fixture, "test_datasources", "[oracle][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(oracle_fixture, "test_driver", "[oracle][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(oracle_fixture, "test_driver_info", "[oracle][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(oracle_fixture, "test_error", "[oracle][error]")
{
    test_error();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_error_message_carries_each_diagnostic_once",
    "[oracle][error]")
{
    test_error_message_carries_each_diagnostic_once();
}

TEST_CASE_METHOD(oracle_fixture, "test_exception", "[oracle][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(oracle_fixture, "test_execute_direct_batch_ops", "[oracle][statement][batch]")
{
    test_execute_direct_batch_ops();
}

TEST_CASE_METHOD(oracle_fixture, "test_execute_multiple", "[oracle][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_execute_multiple_transaction",
    "[oracle][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_execute_prepared_statement_repeatedly",
    "[oracle][statement][prepare]")
{
    test_execute_prepared_statement_repeatedly();
}

TEST_CASE_METHOD(oracle_fixture, "test_get_every_ctype", "[oracle][result][types]")
{
    test_get_every_ctype();
}

TEST_CASE_METHOD(oracle_fixture, "test_get_info", "[oracle][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(oracle_fixture, "test_get_info_widest", "[oracle][metadata][info]")
{
    test_get_info_widest();
}

TEST_CASE_METHOD(oracle_fixture, "test_handle_copy_move_and_swap", "[oracle][handle]")
{
    test_handle_copy_move_and_swap();
}

TEST_CASE_METHOD(oracle_fixture, "test_integral_small_types", "[oracle][integral]")
{
    test_integral_small_types();
}

TEST_CASE_METHOD(oracle_fixture, "test_integral_small_types_batch", "[oracle][integral][batch]")
{
    test_integral_small_types_batch();
}

TEST_CASE_METHOD(oracle_fixture, "test_integral_to_string_conversion", "[oracle][integral]")
{
    test_integral_to_string_conversion();
}

TEST_CASE_METHOD(oracle_fixture, "test_is_null_binary", "[oracle][binary][null]")
{
    test_is_null_binary();
}

TEST_CASE_METHOD(oracle_fixture, "test_just_execute_forms", "[oracle][execute]")
{
    test_just_execute_forms();
}

TEST_CASE_METHOD(oracle_fixture, "test_long_text_chunk_boundaries", "[oracle][result][string]")
{
    test_long_text_chunk_boundaries();
}

TEST_CASE_METHOD(oracle_fixture, "test_move", "[oracle][move]")
{
    test_move();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_nested_transaction_rollback",
    "[oracle][transaction][rollback]")
{
    test_nested_transaction_rollback();
}

TEST_CASE_METHOD(oracle_fixture, "test_null", "[oracle][null]")
{
    test_null();
}

TEST_CASE_METHOD(oracle_fixture, "test_null_long_text_fallback", "[oracle][result][null]")
{
    test_null_long_text_fallback();
}

TEST_CASE_METHOD(oracle_fixture, "test_null_with_bound_columns_unbound", "[oracle][null][unbound]")
{
    test_null_with_bound_columns_unbound();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_parameter_metadata_before_description",
    "[oracle][statement]")
{
    test_parameter_metadata_before_description();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_accessors", "[oracle][result][accessors]")
{
    test_result_accessors();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_at_end", "[oracle][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_column_metadata", "[oracle][result][metadata]")
{
    test_result_column_metadata();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_iterator", "[oracle][result][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_iterator_post_increment", "[oracle][looping]")
{
    test_result_iterator_post_increment();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_rowset_navigation", "[oracle][result][rowset]")
{
    test_result_rowset_navigation();
}

TEST_CASE_METHOD(oracle_fixture, "test_scrollable_cursor", "[oracle][result][cursor][scroll]")
{
    test_scrollable_cursor();
}

TEST_CASE_METHOD(oracle_fixture, "test_result_unbind", "[oracle][result][unbind]")
{
    test_result_unbind();
}

TEST_CASE_METHOD(oracle_fixture, "test_simple", "[oracle]")
{
    test_simple();
}

TEST_CASE_METHOD(oracle_fixture, "test_statement_cancel", "[oracle][statement]")
{
    test_statement_cancel();
}

TEST_CASE_METHOD(oracle_fixture, "test_statement_open_close", "[oracle][statement]")
{
    test_statement_open_close();
}

TEST_CASE_METHOD(oracle_fixture, "test_affected_rows_counts", "[oracle][statement][affected_rows]")
{
    test_affected_rows_counts();
}

TEST_CASE_METHOD(oracle_fixture, "test_statement_parameter_description", "[oracle][statement]")
{
    test_statement_parameter_description();
}

TEST_CASE_METHOD(oracle_fixture, "test_statement_timeout", "[oracle][statement]")
{
    test_statement_timeout();
}

TEST_CASE_METHOD(oracle_fixture, "test_statement_usable_when_result_gone", "[oracle][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(oracle_fixture, "test_std_optional", "[oracle][optional]")
{
    test_std_optional();
}

TEST_CASE_METHOD(oracle_fixture, "test_string", "[oracle][string]")
{
    test_string();
}

TEST_CASE_METHOD(oracle_fixture, "test_string_aggregate", "[oracle][result][string]")
{
    test_string_aggregate();
}

TEST_CASE_METHOD(oracle_fixture, "test_string_vector", "[oracle][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(oracle_fixture, "test_string_view_vector", "[oracle][string]")
{
    test_string_view_vector();
}

TEST_CASE_METHOD(oracle_fixture, "test_timeouts", "[oracle][statement]")
{
    test_timeouts();
}

TEST_CASE_METHOD(oracle_fixture, "test_transaction", "[oracle][transaction]")
{
    test_transaction();
}

TEST_CASE_METHOD(oracle_fixture, "test_while_next_iteration", "[oracle][looping]")
{
    test_while_next_iteration();
}

TEST_CASE_METHOD(oracle_fixture, "test_while_not_end_iteration", "[oracle][looping]")
{
    test_while_not_end_iteration();
}

#if defined(_MSC_VER)

TEST_CASE_METHOD(oracle_fixture, "test_win32_variant", "[oracle][variant][windows]")
{
    test_win32_variant();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_win32_variant_null_literal",
    "[oracle][variant][windows][null]")
{
    test_win32_variant_null_literal();
}

TEST_CASE_METHOD(
    oracle_fixture,
    "test_win32_variant_row_cached_result",
    "[oracle][variant][windows]")
{
    test_win32_variant_row_cached_result();
}

#endif // _MSC_VER
