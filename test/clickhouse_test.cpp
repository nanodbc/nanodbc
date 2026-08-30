#include "test_case_fixture.h"

#include <cstdint>
#include <vector>

namespace
{
struct clickhouse_fixture : public test_case_fixture
{
    clickhouse_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_CLICKHOUSE");
    }

    // The driver answers SQLDescribeParam with SQL_UNKNOWN_TYPE rather than failing, so
    // nanodbc has no type to bind with and ClickHouse rejects the parameter as `Nothing`.
    // Every test here that binds a parameter says what it is binding first, which is what
    // statement::describe_parameters is for.
    static void describe(
        nanodbc::statement& statement,
        std::vector<short> const& type,
        std::vector<unsigned long> const& size)
    {
        std::vector<short> index(type.size());
        for (std::size_t i = 0; i < index.size(); ++i)
            index[i] = static_cast<short>(i);
        statement.describe_parameters(index, type, size, std::vector<short>(type.size(), 0));
    }
};
} // namespace

// Test cases shared with the other backends.
//
// The set is smaller than theirs, and what is missing is missing because the driver or the
// server cannot do it rather than because it went unconsidered: array binding, which the
// driver takes one row of; transactions, which ClickHouse does not have; views and schemas,
// which the driver does not report; and any column that has to hold a null, which needs a
// Nullable type rather than the plain one the shared table definitions use. Each of those
// is covered below by a test of its own that states what does happen.

TEST_CASE_METHOD(clickhouse_fixture, "test_driver", "[clickhouse][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_driver_info", "[clickhouse][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_datasources", "[clickhouse][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(
    clickhouse_fixture,
    "test_catalog_list_catalogs",
    "[clickhouse][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(
    clickhouse_fixture,
    "test_catalog_list_table_types",
    "[clickhouse][catalog][table_types]")
{
    test_catalog_list_table_types();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_connection_environment", "[clickhouse][connection]")
{
    test_connection_environment();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_dbms_info", "[clickhouse][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_get_info", "[clickhouse][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_execute_multiple", "[clickhouse][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_move", "[clickhouse][move]")
{
    test_move();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_result_at_end", "[clickhouse][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_result_iterator", "[clickhouse][result][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(
    clickhouse_fixture,
    "test_statement_usable_when_result_gone",
    "[clickhouse][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_while_not_end_iteration", "[clickhouse][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(clickhouse_fixture, "test_while_next_iteration", "[clickhouse][looping]")
{
    test_while_next_iteration();
}

// ClickHouse-specific test cases.

TEST_CASE_METHOD(clickhouse_fixture, "test_vendor", "[clickhouse][dmbs][metadata][info]")
{
    auto connection = connect();
    REQUIRE(vendor_ == database_vendor::clickhouse);
}

// The parameter description the driver will not give.
TEST_CASE_METHOD(
    clickhouse_fixture,
    "test_parameter_not_described",
    "[clickhouse][statement][bind]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_parameter_not_described"), NANODBC_TEXT("(i int)"));

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_parameter_not_described (i) values (?)"));
    REQUIRE(statement.parameters() == 1);

    // SQLDescribeParam succeeds and says nothing, which is the whole difficulty: a driver
    // that failed the call outright would at least raise it here.
    REQUIRE(statement.parameter_type(0) == SQL_UNKNOWN_TYPE);
    REQUIRE(statement.parameter_size(0) == 0);
    REQUIRE(statement.parameter_scale(0) == 0);

    // Binding on that description reaches the server as an untyped parameter, which it
    // refuses. The tests below describe the parameter first, and none of them sees this.
    int value = 1;
    statement.bind(0, &value);
    REQUIRE_THROWS_AS(nanodbc::just_execute(statement), nanodbc::database_error);
}

TEST_CASE_METHOD(clickhouse_fixture, "test_bind_integral", "[clickhouse][bind][integral]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_bind_integral"),
        NANODBC_TEXT("(i int, b bigint, f double)"));

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_bind_integral (i, b, f) values (?, ?, ?)"));
    REQUIRE(statement.parameters() == 3);
    describe(statement, {SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE}, {10, 19, 15});

    std::int32_t const i = -2147483648LL + 1;
    std::int64_t const b = 9007199254740993LL; // more digits than a double holds exactly
    double const f = 3.25;                     // exact in binary, so it compares equal
    statement.bind(0, &i);
    statement.bind(1, &b);
    statement.bind(2, &f);
    nanodbc::just_execute(statement);

    auto results =
        nanodbc::execute(connection, NANODBC_TEXT("select i, b, f from test_bind_integral"));
    REQUIRE(results.next());
    REQUIRE(results.get<std::int32_t>(0) == i);
    REQUIRE(results.get<std::int64_t>(1) == b);
    REQUIRE(results.get<double>(2) == f);
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(clickhouse_fixture, "test_bind_string", "[clickhouse][bind][string]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_bind_string"), NANODBC_TEXT("(s varchar(60))"));

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_bind_string (s) values (?)"));
    describe(statement, {SQL_VARCHAR}, {60});

    auto const value = NANODBC_TEXT("this is a test");
    statement.bind(0, value);
    nanodbc::just_execute(statement);

    auto results = nanodbc::execute(connection, NANODBC_TEXT("select s from test_bind_string"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == nanodbc::string(value));
}

// The round trip the backend was added for: ClickHouse types an unbounded varchar as
// String, so a value far longer than the column was declared with survives it.
TEST_CASE_METHOD(clickhouse_fixture, "test_bind_long_string", "[clickhouse][bind][string]")
{
    auto connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_bind_long_string"), NANODBC_TEXT("(s varchar(60))"));

    auto const value = create_random_string<nanodbc::string>(3000, 3000);
    REQUIRE(value.size() == 3000);

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_bind_long_string (s) values (?)"));
    describe(statement, {SQL_VARCHAR}, {static_cast<unsigned long>(value.size())});
    statement.bind(0, value.c_str());
    nanodbc::just_execute(statement);

    auto results = nanodbc::execute(
        connection, NANODBC_TEXT("select s, length(s) from test_bind_long_string"));
    REQUIRE(results.next());
    REQUIRE(results.get<std::int64_t>(1) == 3000);
    REQUIRE(results.get<nanodbc::string>(0) == value);
}

TEST_CASE_METHOD(clickhouse_fixture, "test_bind_date_and_timestamp", "[clickhouse][bind][time]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_bind_date_and_timestamp"),
        NANODBC_TEXT("(d date, ts timestamp)"));

    nanodbc::statement statement(connection);
    prepare(
        statement, NANODBC_TEXT("insert into test_bind_date_and_timestamp (d, ts) values (?, ?)"));
    describe(statement, {SQL_DATE, SQL_TIMESTAMP}, {10, 19});

    nanodbc::date date{2026, 8, 30};
    nanodbc::timestamp timestamp{2026, 8, 30, 11, 45, 59, 0};
    statement.bind(0, &date);
    statement.bind(1, &timestamp);
    nanodbc::just_execute(statement);

    auto results = nanodbc::execute(
        connection, NANODBC_TEXT("select d, ts from test_bind_date_and_timestamp"));
    REQUIRE(results.next());

    auto const d = results.get<nanodbc::date>(0);
    REQUIRE(d.year == 2026);
    REQUIRE(d.month == 8);
    REQUIRE(d.day == 30);

    // ClickHouse's DateTime counts whole seconds, so the fraction the timestamp carries is
    // not part of the round trip.
    auto const ts = results.get<nanodbc::timestamp>(1);
    REQUIRE(ts.year == 2026);
    REQUIRE(ts.month == 8);
    REQUIRE(ts.day == 30);
    REQUIRE(ts.hour == 11);
    REQUIRE(ts.min == 45);
    REQUIRE(ts.sec == 59);
}

// A column holds a null only if its type says so. ClickHouse spells that Nullable(T), and
// a plain column asked to take a null quietly stores the type's default instead, which is
// why the shared null tests, whose tables are declared with plain types, are not run here.
TEST_CASE_METHOD(clickhouse_fixture, "test_null_needs_a_nullable_column", "[clickhouse][null]")
{
    auto connection = connect();

    create_table(
        connection,
        NANODBC_TEXT("test_null_needs_a_nullable_column"),
        NANODBC_TEXT("(plain int, nullable Nullable(Int32))"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_null_needs_a_nullable_column values (null, null);"));

    auto results = nanodbc::execute(
        connection, NANODBC_TEXT("select plain, nullable from test_null_needs_a_nullable_column"));
    REQUIRE(results.next());

    // The null the plain column was given is gone, and a zero stands in its place.
    REQUIRE(!results.is_null(0));
    REQUIRE(results.get<int>(0) == 0);

    REQUIRE(results.is_null(1));
    REQUIRE(results.get<int>(1, -1) == -1);
    REQUIRE(results.get<nanodbc::string>(1, NANODBC_TEXT("null")) == NANODBC_TEXT("null"));
    REQUIRE_THROWS_AS(results.get<int>(1), nanodbc::null_access_error);
}

TEST_CASE_METHOD(clickhouse_fixture, "test_bind_null", "[clickhouse][bind][null]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_bind_null"),
        NANODBC_TEXT("(i Nullable(Int32), s Nullable(String))"));

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_bind_null (i, s) values (?, ?)"));
    describe(statement, {SQL_INTEGER, SQL_VARCHAR}, {10, 60});
    statement.bind_null(0);
    statement.bind_null(1);
    nanodbc::just_execute(statement);

    auto results = nanodbc::execute(connection, NANODBC_TEXT("select i, s from test_bind_null"));
    REQUIRE(results.next());
    REQUIRE(results.is_null(0));
    REQUIRE(results.is_null(1));
    REQUIRE(!results.next());
}

// A bound array reaches the server as its first element and the rest is dropped, without
// a diagnostic to say so. Covered so that the day the driver learns to bind an array, this
// fails and the shared batch tests can be turned on for ClickHouse too.
TEST_CASE_METHOD(clickhouse_fixture, "test_batch_binds_one_row", "[clickhouse][batch]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_batch_binds_one_row"), NANODBC_TEXT("(i int)"));

    std::size_t const batch_size = 5;
    int values[batch_size] = {1, 2, 3, 4, 5};

    nanodbc::statement statement(connection);
    prepare(statement, NANODBC_TEXT("insert into test_batch_binds_one_row (i) values (?)"));
    describe(statement, {SQL_INTEGER}, {10});
    statement.bind(0, values, batch_size);
    nanodbc::just_execute(statement, batch_size);

    auto results = nanodbc::execute(
        connection,
        NANODBC_TEXT("select toInt32(count()), toInt32(sum(i)) from test_batch_binds_one_row"));
    REQUIRE(results.next());
    REQUIRE(results.get<int>(0) == 1);
    REQUIRE(results.get<int>(1) == values[0]);
}

// ClickHouse has no transactions, and the driver says so through SQLGetInfo rather than by
// refusing the calls: a commit is accepted and a rollback leaves the work in place.
TEST_CASE_METHOD(clickhouse_fixture, "test_transaction_is_not_one", "[clickhouse][transaction]")
{
    auto connection = connect();
    REQUIRE(connection.get_info<unsigned short>(SQL_TXN_CAPABLE) == SQL_TC_NONE);

    create_table(connection, NANODBC_TEXT("test_transaction_is_not_one"), NANODBC_TEXT("(i int)"));
    execute(connection, NANODBC_TEXT("insert into test_transaction_is_not_one values (1);"));

    {
        nanodbc::transaction transaction(connection);
        execute(connection, NANODBC_TEXT("insert into test_transaction_is_not_one values (2);"));
        transaction.rollback();
    }

    auto results = nanodbc::execute(
        connection, NANODBC_TEXT("select toInt32(count()) from test_transaction_is_not_one"));
    REQUIRE(results.next());
    REQUIRE(results.get<int>(0) == 2); // the rolled back row is still there
}

// A ClickHouse database stands where a schema would, and the driver reports it as a
// catalog, leaving the schema list empty.
TEST_CASE_METHOD(
    clickhouse_fixture,
    "test_catalog_has_no_schemas",
    "[clickhouse][catalog][schemas]")
{
    auto connection = connect();
    nanodbc::catalog catalog(connection);

    REQUIRE(catalog.list_schemas().empty());

    auto const catalogs = catalog.list_catalogs();
    REQUIRE(!catalogs.empty());
    REQUIRE(
        std::find(catalogs.cbegin(), catalogs.cend(), NANODBC_TEXT("system")) != catalogs.cend());
}

// Only tables are reported. A view exists and can be selected from, but SQLTables does not
// list it and VIEW is not among the table types the driver names.
TEST_CASE_METHOD(clickhouse_fixture, "test_catalog_tables_only", "[clickhouse][catalog][tables]")
{
    auto connection = connect();
    nanodbc::catalog catalog(connection);

    nanodbc::string const table_name(NANODBC_TEXT("test_catalog_tables_only"));
    nanodbc::string const view_name(NANODBC_TEXT("test_catalog_tables_only_view"));
    create_table(connection, table_name, NANODBC_TEXT("(i int)"));
    drop_table(connection, view_name, true);
    execute(
        connection,
        NANODBC_TEXT("create view ") + view_name + NANODBC_TEXT(" as select i from ") + table_name);

    {
        auto const types = catalog.list_table_types();
        REQUIRE(std::find(types.cbegin(), types.cend(), NANODBC_TEXT("TABLE")) != types.cend());
        REQUIRE(std::find(types.cbegin(), types.cend(), NANODBC_TEXT("VIEW")) == types.cend());
    }

    {
        auto tables = catalog.find_tables(table_name);
        REQUIRE(tables.next());
        REQUIRE(tables.table_name() == table_name);
        REQUIRE(tables.table_type() == NANODBC_TEXT("TABLE"));
        REQUIRE(!tables.table_catalog().empty()); // the database the connection opened
        REQUIRE(tables.table_schema().empty());
        REQUIRE(!tables.next());
    }

    {
        auto views = catalog.find_tables(view_name, NANODBC_TEXT("VIEW"));
        REQUIRE(!views.next());
    }

    // The view is there all the same, which is what makes its absence the driver's doing.
    auto results =
        nanodbc::execute(connection, NANODBC_TEXT("select toInt32(count()) from ") + view_name);
    REQUIRE(results.next());
}

TEST_CASE_METHOD(clickhouse_fixture, "test_catalog_columns", "[clickhouse][catalog][columns]")
{
    auto connection = connect();
    nanodbc::catalog catalog(connection);

    nanodbc::string const table_name(NANODBC_TEXT("test_catalog_columns"));
    create_table(
        connection,
        table_name,
        NANODBC_TEXT(
            "(c0 int, c1 smallint, c2 decimal(9, 3), c3 date, c4 varchar(60), "
            "c5 Nullable(Int32))"));

    auto columns = catalog.find_columns(NANODBC_TEXT("%"), table_name);

    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c0"));
    REQUIRE(columns.table_name() == table_name);
    REQUIRE(columns.ordinal_position() == 1);
    REQUIRE(columns.sql_data_type() == SQL_INTEGER);
    REQUIRE(columns.type_name() == NANODBC_TEXT("Int32"));
    // The reported size counts the sign among an Int32's eleven characters.
    REQUIRE(columns.column_size() == 11);
    REQUIRE(columns.decimal_digits() == 0);
    REQUIRE(columns.nullable() == SQL_NO_NULLS);
    REQUIRE(columns.is_nullable() == NANODBC_TEXT("NO"));

    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c1"));
    REQUIRE(columns.sql_data_type() == SQL_SMALLINT);
    REQUIRE(columns.column_size() == 6);

    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c2"));
    REQUIRE(columns.sql_data_type() == SQL_DECIMAL);
    REQUIRE(columns.column_size() == 9);
    REQUIRE(columns.decimal_digits() == 3);

    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c3"));
    REQUIRE(columns.data_type() == SQL_TYPE_DATE);
    REQUIRE(columns.column_size() == 10);

    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c4"));
    REQUIRE(columns.sql_data_type() == SQL_VARCHAR);
    REQUIRE(columns.type_name() == NANODBC_TEXT("String"));

    // Only the column declared Nullable is reported as one.
    REQUIRE(columns.next());
    REQUIRE(columns.column_name() == NANODBC_TEXT("c5"));
    REQUIRE(columns.nullable() == SQL_NULLABLE);
    REQUIRE(columns.is_nullable() == NANODBC_TEXT("YES"));

    REQUIRE(!columns.next());
}

// The shared decimal test expects the scale the column was declared with to be part of the
// text; ClickHouse's driver formats the value and drops what is trailing.
TEST_CASE_METHOD(clickhouse_fixture, "test_decimal_conversion", "[clickhouse][decimal][conversion]")
{
    auto connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_decimal_conversion"), NANODBC_TEXT("(d decimal(9, 3))"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (12345.987);"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (5.600);"));
    execute(connection, NANODBC_TEXT("insert into test_decimal_conversion values (-1.333);"));

    auto results = nanodbc::execute(
        connection, NANODBC_TEXT("select d from test_decimal_conversion order by 1 desc;"));

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("12345.987"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("5.6"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == NANODBC_TEXT("-1.333"));
}
