#include "test_case_fixture.h"

#include <cstdint>

namespace
{
struct firebird_fixture : public test_case_fixture
{
    firebird_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_FIREBIRD");
    }
};
} // namespace

// Test cases shared with the other backends.
//
// The set is smaller than Vertica's, and what is absent is absent for one of three
// reasons, each covered by a test of its own below: the driver drops all but the first row
// of a bound array, it answers the catalog in UTF-16 through its ANSI entry points, and
// Firebird wants a FROM clause where the shared cases select a bare value.

TEST_CASE_METHOD(firebird_fixture, "test_driver", "[firebird][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(firebird_fixture, "test_driver_info", "[firebird][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(firebird_fixture, "test_datasources", "[firebird][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(firebird_fixture, "test_catalog_list_catalogs", "[firebird][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(firebird_fixture, "test_catalog_list_schemas", "[firebird][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(
    firebird_fixture,
    "test_catalog_list_table_types",
    "[firebird][catalog][table_types]")
{
    test_catalog_list_table_types();
}

TEST_CASE_METHOD(firebird_fixture, "test_connection_environment", "[firebird][connection]")
{
    test_connection_environment();
}

TEST_CASE_METHOD(firebird_fixture, "test_dbms_info", "[firebird][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(firebird_fixture, "test_get_info", "[firebird][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(firebird_fixture, "test_decimal_conversion", "[firebird][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(firebird_fixture, "test_error", "[firebird][error]")
{
    test_error();
}

TEST_CASE_METHOD(firebird_fixture, "test_exception", "[firebird][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(firebird_fixture, "test_integral", "[firebird][integral]")
{
    test_integral<firebird_fixture>();
}

TEST_CASE_METHOD(firebird_fixture, "test_integral_small_types", "[firebird][integral]")
{
    test_integral_small_types();
}

TEST_CASE_METHOD(firebird_fixture, "test_move", "[firebird][move]")
{
    test_move();
}

TEST_CASE_METHOD(firebird_fixture, "test_result_at_end", "[firebird][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(firebird_fixture, "test_result_iterator", "[firebird][result][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(firebird_fixture, "test_simple", "[firebird]")
{
    test_simple();
}

TEST_CASE_METHOD(
    firebird_fixture,
    "test_statement_usable_when_result_gone",
    "[firebird][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(firebird_fixture, "test_string", "[firebird][string]")
{
    test_string();
}

TEST_CASE_METHOD(firebird_fixture, "test_time", "[firebird][time]")
{
    test_time();
}

TEST_CASE_METHOD(firebird_fixture, "test_blob_binary", "[firebird][blob][binary]")
{
    test_blob_binary();
}

TEST_CASE_METHOD(firebird_fixture, "test_while_not_end_iteration", "[firebird][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(firebird_fixture, "test_while_next_iteration", "[firebird][looping]")
{
    test_while_next_iteration();
}

// Firebird-specific test cases.

TEST_CASE_METHOD(firebird_fixture, "test_vendor", "[firebird][dmbs][metadata][info]")
{
    auto connection = connect();
    REQUIRE(vendor_ == database_vendor::firebird);
    // Unquoted identifiers fold upwards, which is what as_identifier() answers for.
    REQUIRE(connection.get_info<unsigned short>(SQL_IDENTIFIER_CASE) == SQL_IC_UPPER);
    // Unlike ClickHouse, Firebird has transactions and the driver says so.
    REQUIRE(connection.get_info<unsigned short>(SQL_TXN_CAPABLE) != SQL_TC_NONE);
}

// A bound array reaches the server as its first element unless SQL_ATTR_PARAMSET_SIZE was
// set before the parameters were bound, which nanodbc sets after. The driver takes the
// size at bind time, reports SQL_SUCCESS, and leaves the rest of the array unsent, so the
// loss is silent. This is why none of the shared batch cases is run here.
//
// Setting the attribute first, which is what the ODBC specification allows and this driver
// requires, inserts every row: the two orders are covered together so that the day the
// driver stops caring, this fails and the shared cases can be turned on.
TEST_CASE_METHOD(firebird_fixture, "test_batch_binds_one_row", "[firebird][batch]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_batch_binds_one_row"), NANODBC_TEXT("(i integer)"));

    std::size_t const batch_size = 5;
    int values[batch_size] = {10, 20, 30, 40, 50};

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_batch_binds_one_row (i) values (?)"));
    statement.bind(0, values, batch_size);
    nanodbc::just_execute(statement, batch_size);

    auto results =
        nanodbc::execute(connection, NANODBC_TEXT("select count(*) from test_batch_binds_one_row"));
    REQUIRE(results.next());
    REQUIRE(results.get<int>(0) == 1); // the other four were dropped without a diagnostic
}

// The driver exports both the ANSI and the Unicode entry points, so unixODBC hands a
// narrow application the ANSI ones, and those answer the catalog in UTF-16 all the same.
// A name therefore comes back with a null after every character, which is why the shared
// catalog cases that read a name are not run here.
TEST_CASE_METHOD(firebird_fixture, "test_catalog_names_come_back_wide", "[firebird][catalog]")
{
    auto connection = connect();
    nanodbc::catalog catalog(connection);

    nanodbc::string const table_name(NANODBC_TEXT("TEST_CATALOG_NAMES"));
    create_table(connection, table_name, NANODBC_TEXT("(i integer)"));

    // The driver reads the name it is given correctly; it is the answer that is wide.
    auto tables = catalog.find_tables(table_name);
    REQUIRE(tables.next());

    auto const reported = tables.table_name();
    REQUIRE(reported != table_name);
    // 'T', 0, 'E', 0, ... - the UTF-16 of the name, one byte per character of the answer.
    REQUIRE(reported.size() == table_name.size() * 2);
    for (std::size_t i = 0; i < table_name.size(); ++i)
    {
        REQUIRE(reported[i * 2] == table_name[i]);
        REQUIRE(reported[i * 2 + 1] == NANODBC_TEXT('\0'));
    }
}

// Firebird has no bare select: a value comes from RDB$DATABASE, the one row every database
// carries. The shared execute cases select a bare value, so they are not run here.
TEST_CASE_METHOD(firebird_fixture, "test_select_wants_a_from", "[firebird][execute]")
{
    auto connection = connect();

    REQUIRE_THROWS_AS(execute(connection, NANODBC_TEXT("select 42;")), nanodbc::database_error);

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("select 42 from rdb$database;"));
    for (int i = 0; i < 3; ++i)
    {
        auto results = statement.execute();
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 42);
    }
}
