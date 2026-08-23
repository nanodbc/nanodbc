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

    // Whether the driver can report the implementation row descriptor of a prepared but
    // not yet executed statement, which is how the IRD tests below use it. Some
    // Connector/ODBC versions answer SQL_DESC_COUNT with zero however many columns the
    // statement has, so this is probed the same way the tests use it.
    bool supports_implementation_row_descriptor()
    {
        try
        {
            auto c = connect();
            nanodbc::statement s(c, NANODBC_TEXT("select 1;"));
            nanodbc::implementation_row_descriptor ird(s);
            return ird.count() > 0;
        }
        catch (nanodbc::database_error const&)
        {
            return false;
        }
    }
};
} // namespace

TEST_CASE_METHOD(mysql_fixture, "test_driver", "[mysql][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(mysql_fixture, "test_driver_info", "[mysql][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(mysql_fixture, "test_datasources", "[mysql][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(mysql_fixture, "test_affected_rows", "[mysql][affected_rows]")
{
    nanodbc::connection conn = connect();

    // CREATE DATABASE|TABLE
    {
        execute(conn, NANODBC_TEXT("DROP DATABASE IF EXISTS nanodbc_test_temp_db"));
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("CREATE DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.has_affected_rows());
        REQUIRE(result.affected_rows() == 1);
        execute(conn, NANODBC_TEXT("USE nanodbc_test_temp_db"));
        result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
        REQUIRE(result.affected_rows() == 0);
    }
    // INSERT
    {
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (1)"));
        REQUIRE(result.has_affected_rows());
        REQUIRE(result.affected_rows() == 1);
        result = execute(conn, NANODBC_TEXT("INSERT INTO nanodbc_test_temp_table VALUES (2)"));
        REQUIRE(result.affected_rows() == 1);
    }
    // SELECT
    {
        auto result = execute(conn, NANODBC_TEXT("SELECT i FROM nanodbc_test_temp_table"));
        REQUIRE(result.has_affected_rows());
        REQUIRE(result.affected_rows() == 2);
    }
    // DELETE
    {
        auto result = execute(conn, NANODBC_TEXT("DELETE FROM nanodbc_test_temp_table"));
        REQUIRE(result.affected_rows() == 2);
    }
    // Inserting/retrieving long strings
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

TEST_CASE_METHOD(
    mysql_fixture,
    "test_rowset_size_apart_from_parameter_sets",
    "[mysql][result][rowset]")
{
    test_rowset_size_apart_from_parameter_sets();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_delete", "[mysql][batch][delete]")
{
    test_batch_delete();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_null", "[mysql][batch][null]")
{
    test_batch_insert_null();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_string", "[mysql][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(mysql_fixture, "test_batch_insert_mixed", "[mysql][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_rows", "[mysql][batch][bind_rows]")
{
    test_bind_rows();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_rows_null", "[mysql][batch][bind_rows]")
{
    test_bind_rows_null();
}

TEST_CASE_METHOD(mysql_fixture, "test_std_optional", "[mysql][optional]")
{
    test_std_optional();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_bind_timestamp_as_string",
    "[mysql][statement][bind][timestamp]")
{
    test_bind_timestamp_as_string();
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

/*
 * 4/14/23: Disabled since windows mariadb (10.3) pipeline
 * fails.
TEST_CASE_METHOD(mysql_fixture, "test_catalog_list_table_types", "[mysql][catalog][table_types]")
{
    test_catalog_list_table_types();
}
*/

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

// The MySQL driver answers SQLTablePrivileges with an empty result set however the
// privileges are granted, so test_catalog_table_privileges has nothing to find here.

TEST_CASE_METHOD(mysql_fixture, "test_column_descriptor", "[mysql][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(mysql_fixture, "test_connection_environment", "[mysql][connection]")
{
    test_connection_environment();
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

TEST_CASE_METHOD(mysql_fixture, "test_error", "[mysql][error]")
{
    test_error();
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

TEST_CASE_METHOD(mysql_fixture, "test_implementation_row_descriptor", "[mysql][descriptor][ird]")
{
    if (!supports_implementation_row_descriptor())
    {
        WARN("skipped: driver does not implement the implementation row descriptor");
        return;
    }

    test_implementation_row_descriptor();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_implementation_row_descriptor_with_expressions",
    "[mysql][descriptor][ird]")
{
    if (!supports_implementation_row_descriptor())
    {
        WARN("skipped: driver does not implement the implementation row descriptor");
        return;
    }

    test_implementation_row_descriptor_with_expressions();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_implementation_row_descriptor_auto_unique_value",
    "[mysql][descriptor][ird]")
{
    if (!supports_implementation_row_descriptor())
    {
        WARN("skipped: driver does not implement the implementation row descriptor");
        return;
    }

    auto c = connect();

    create_table(
        c, NANODBC_TEXT("test_implementation_row_descriptor_auto_unique_value"), NANODBC_TEXT(R"(
fid int NOT NULL AUTO_INCREMENT,
name varchar(60),
PRIMARY KEY(fid)
)"));

    auto const sql =
        NANODBC_TEXT("SELECT fid, name FROM test_implementation_row_descriptor_auto_unique_value");
    nanodbc::statement s(c, sql);
    nanodbc::implementation_row_descriptor ird(s);
    REQUIRE(ird.count() == 2);
    REQUIRE(ird.auto_unique_value(0));
    REQUIRE(!ird.auto_unique_value(1));
}

TEST_CASE_METHOD(mysql_fixture, "test_integral", "[mysql][integral]")
{
    test_integral<mysql_fixture>();
}

TEST_CASE_METHOD(mysql_fixture, "test_integral_small_types", "[mysql][integral]")
{
    test_integral_small_types();
}

TEST_CASE_METHOD(mysql_fixture, "test_integral_small_types_batch", "[mysql][integral][batch]")
{
    test_integral_small_types_batch();
}

TEST_CASE_METHOD(mysql_fixture, "test_integral_to_string_conversion", "[mysql][integral]")
{
    test_integral_to_string_conversion();
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

TEST_CASE_METHOD(mysql_fixture, "test_null_with_bound_columns_unbound", "[mysql][null][unbound]")
{
    test_null_with_bound_columns_unbound();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_at_end", "[mysql][result][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(mysql_fixture, "test_temporal_conversions", "[mysql][date][time][timestamp]")
{
    test_temporal_conversions();
}

TEST_CASE_METHOD(mysql_fixture, "test_statement_open_close", "[mysql][statement]")
{
    test_statement_open_close();
}

TEST_CASE_METHOD(mysql_fixture, "test_affected_rows_counts", "[mysql][statement][affected_rows]")
{
    test_affected_rows_counts();
}

TEST_CASE_METHOD(mysql_fixture, "test_boolean_column", "[mysql][boolean]")
{
    test_boolean_column();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_null_array", "[mysql][null]")
{
    test_bind_null_array();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_null_sentry", "[mysql][statement]")
{
    test_bind_null_sentry();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_binary_null_sentry", "[mysql][binary][null]")
{
    test_bind_binary_null_sentry();
}

TEST_CASE_METHOD(mysql_fixture, "test_timeouts", "[mysql][statement]")
{
    test_timeouts();
}

TEST_CASE_METHOD(mysql_fixture, "test_statement_parameter_description", "[mysql][statement]")
{
    test_statement_parameter_description();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_rowset_navigation", "[mysql][result][rowset]")
{
    test_result_rowset_navigation();
}

TEST_CASE_METHOD(mysql_fixture, "test_scrollable_cursor", "[mysql][result][cursor][scroll]")
{
    test_scrollable_cursor();
}

TEST_CASE_METHOD(mysql_fixture, "test_execute_direct_batch_ops", "[mysql][statement][batch]")
{
    test_execute_direct_batch_ops();
}

TEST_CASE_METHOD(mysql_fixture, "test_string_aggregate", "[mysql][result][string]")
{
    test_string_aggregate();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_execute_prepared_statement_repeatedly",
    "[mysql][statement][prepare]")
{
    test_execute_prepared_statement_repeatedly();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_arithmetic_null_sentry", "[mysql][bind][null]")
{
    test_bind_arithmetic_null_sentry();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_nested_transaction_rollback",
    "[mysql][transaction][rollback]")
{
    test_nested_transaction_rollback();
}

TEST_CASE_METHOD(mysql_fixture, "test_long_text_chunk_boundaries", "[mysql][result][string]")
{
    test_long_text_chunk_boundaries();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_unbind", "[mysql][result][unbind]")
{
    test_result_unbind();
}

TEST_CASE_METHOD(mysql_fixture, "test_is_null_binary", "[mysql][binary][null]")
{
    test_is_null_binary();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_accessors", "[mysql][result][accessors]")
{
    test_result_accessors();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_iterator", "[mysql][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(mysql_fixture, "test_simple", "[mysql]")
{
    test_simple();
}

TEST_CASE_METHOD(mysql_fixture, "test_statement_usable_when_result_gone", "[mysql][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(mysql_fixture, "test_string", "[mysql][string]")
{
    test_string();
}

TEST_CASE_METHOD(mysql_fixture, "test_string_vector", "[mysql][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(mysql_fixture, "test_string_view_vector", "[mysql][string]")
{
    test_string_view_vector();
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

#if defined(_MSC_VER)
TEST_CASE_METHOD(mysql_fixture, "test_win32_variant", "[mysql][variant][windows]")
{
    test_win32_variant();
}

TEST_CASE_METHOD(
    mysql_fixture,
    "test_win32_variant_null_literal",
    "[mysql][variant][windows][null]")
{
    test_win32_variant_null_literal();
}

TEST_CASE_METHOD(mysql_fixture, "test_win32_variant_row_cached_result", "[mysql][variant][windows]")
{
    test_win32_variant_row_cached_result();
}
#endif // _MSC_VER

TEST_CASE_METHOD(mysql_fixture, "test_while_not_end_iteration", "[mysql][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(mysql_fixture, "test_while_next_iteration", "[mysql][looping]")
{
    test_while_next_iteration();
}

TEST_CASE_METHOD(mysql_fixture, "test_connection_catalog_name", "[mysql][connection][metadata]")
{
    test_connection_catalog_name();
}

TEST_CASE_METHOD(mysql_fixture, "test_get_info_widest", "[mysql][metadata][info]")
{
    test_get_info_widest();
}

TEST_CASE_METHOD(mysql_fixture, "test_statement_cancel", "[mysql][statement]")
{
    test_statement_cancel();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_iterator_post_increment", "[mysql][looping]")
{
    test_result_iterator_post_increment();
}

TEST_CASE_METHOD(mysql_fixture, "test_result_column_metadata", "[mysql][result][metadata]")
{
    test_result_column_metadata();
}

TEST_CASE_METHOD(mysql_fixture, "test_handle_copy_move_and_swap", "[mysql][handle]")
{
    test_handle_copy_move_and_swap();
}

TEST_CASE_METHOD(mysql_fixture, "test_just_execute_forms", "[mysql][execute]")
{
    test_just_execute_forms();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_every_form", "[mysql][bind][batch]")
{
    test_bind_every_form();
}

TEST_CASE_METHOD(mysql_fixture, "test_get_every_ctype", "[mysql][result][types]")
{
    test_get_every_ctype();
}

// The unsigned C types the bind switch dispatches on. MySQL is the only backend here whose
// integer columns can be UNSIGNED, so it is the only one that binds these.
TEST_CASE_METHOD(mysql_fixture, "test_get_unsigned_ctypes", "[mysql][result][types]")
{
    nanodbc::connection connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_get_unsigned_ctypes"),
        NANODBC_TEXT(
            "(ti tinyint unsigned, si smallint unsigned, i int unsigned, "
            "bi bigint unsigned)"));
    execute(
        connection,
        NANODBC_TEXT(
            "insert into test_get_unsigned_ctypes (ti, si, i, bi) "
            "values (200, 60000, 4000000000, 9000000000000000000);"));

    auto results =
        execute(connection, NANODBC_TEXT("select ti, si, i, bi from test_get_unsigned_ctypes;"));
    REQUIRE(results.next());
    REQUIRE(results.get<unsigned short>(0) == 200);
    REQUIRE(results.get<int>(0) == 200);
    REQUIRE(results.get<unsigned int>(1) == 60000);
    REQUIRE(results.get<long>(1) == 60000);
    REQUIRE(results.get<unsigned long long>(2) == 4000000000ULL);
    REQUIRE(results.get<double>(2) == 4000000000.0);
    REQUIRE(results.get<unsigned long long>(3) == 9000000000000000000ULL);
}

TEST_CASE_METHOD(mysql_fixture, "test_statement_timeout", "[mysql][statement]")
{
    test_statement_timeout();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_wide_strings_with_sentry", "[mysql][bind][unicode]")
{
    test_bind_wide_strings_with_sentry();
}

TEST_CASE_METHOD(mysql_fixture, "test_parameter_metadata_before_description", "[mysql][statement]")
{
    test_parameter_metadata_before_description();
}

TEST_CASE_METHOD(mysql_fixture, "test_binary_read_shapes", "[mysql][result][binary]")
{
    test_binary_read_shapes();
}

TEST_CASE_METHOD(mysql_fixture, "test_null_long_text_fallback", "[mysql][result][null]")
{
    test_null_long_text_fallback();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_date_to_timestamp_parameter", "[mysql][bind][date]")
{
    test_bind_date_to_timestamp_parameter();
}

TEST_CASE_METHOD(mysql_fixture, "test_bind_null_in_single_row_batch", "[mysql][bind][null]")
{
    test_bind_null_in_single_row_batch();
}
