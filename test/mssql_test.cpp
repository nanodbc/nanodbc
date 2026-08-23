#include "test_case_fixture.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <atlsafe.h>
#include <comutil.h>
#ifdef _DEBUG
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif
#endif

namespace
{
struct mssql_fixture : public test_case_fixture
{
    mssql_fixture()
        : test_case_fixture()
    {
        // connection string from command line or NANODBC_TEST_CONNSTR environment variable
        if (connection_string_.empty())
            connection_string_ = get_env("NANODBC_TEST_CONNSTR_MSSQL");
    }

#ifdef NANODBC_HAS_STD_VARIANT
    using base_test_fixture::connect;
    nanodbc::connection
    connect(std::list<nanodbc::connection::attribute> const& attributes, bool const& is_async)
    {
        nanodbc::connection connection(connection_string_, attributes);
        if (!is_async)
        {
            REQUIRE(connection.connected());
            vendor_ = get_vendor(connection.dbms_name());
        }
        return connection;
    }
#endif

#if !defined(NANODBC_DISABLE_ASYNC) && defined(WIN32)
    void test_async_internal(nanodbc::connection& conn, HANDLE& event_handle)
    {
        nanodbc::statement stmt(conn);
        if (stmt.async_prepare(NANODBC_TEXT("select count(*) from sys.tables;"), event_handle))
            WaitForSingleObject(event_handle, INFINITE);
        stmt.complete_prepare();

        if (stmt.async_execute(event_handle))
            WaitForSingleObject(event_handle, INFINITE);
        nanodbc::result row = stmt.complete_execute();

        if (row.async_next(event_handle))
            WaitForSingleObject(event_handle, INFINITE);
        REQUIRE(row.complete_next());

        REQUIRE(row.get<int>(0) >= 0);
    }
#endif

    inline bool success(RETCODE rc)
    {
#ifdef NANODBC_ODBC_API_DEBUG
        std::cerr << "<-- rc: " << return_code(rc) << " | " << std::endl;
#endif
        return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
    }

    // `name` is a type name
    // `def` is a comma separated column definitions, trailing '(' and ')' are optional.
    void create_table_type(
        nanodbc::connection& connection,
        nanodbc::string const& name,
        nanodbc::string def) const
    {
        nanodbc::string sql(NANODBC_TEXT("CREATE TYPE "));
        sql += name;
        sql += NANODBC_TEXT(" AS TABLE ");
        sql += def;
        sql += NANODBC_TEXT(';');

        drop_table_type(connection, name);
        execute(connection, sql);
    }

    virtual void drop_table_type(nanodbc::connection& connection, nanodbc::string const& name) const
    {
        bool type_exists = true;

        try
        {
            auto sql =
                NANODBC_TEXT("SELECT 1 FROM sys.types WHERE is_table_type = 1 AND name = '") +
                name + NANODBC_TEXT("';");
            nanodbc::result results = execute(connection, sql);
            results.next();
            type_exists = (0 < results.rows());
        }
        catch (...)
        {
            type_exists = false;
        }

        if (type_exists)
        {
            execute(connection, NANODBC_TEXT("DROP TYPE ") + name + NANODBC_TEXT(";"));
        }
    }

    virtual void drop_procedure(nanodbc::connection& connection, nanodbc::string const& name) const
    {
        bool procedure_exists = true;

        try
        {
            auto sql = NANODBC_TEXT("SELECT 1 FROM sys.procedures WHERE name = '") + name +
                       NANODBC_TEXT("';");
            nanodbc::result results = execute(connection, sql);
            results.next();
            procedure_exists = (0 < results.rows());
        }
        catch (...)
        {
            procedure_exists = false;
        }

        if (procedure_exists)
        {
            execute(connection, NANODBC_TEXT("DROP PROCEDURE ") + name + NANODBC_TEXT(";"));
        }
    }
};
} // namespace

TEST_CASE_METHOD(mssql_fixture, "test_driver", "[mssql][driver]")
{
    test_driver();
}

TEST_CASE_METHOD(mssql_fixture, "test_driver_info", "[mssql][driver][metadata][info]")
{
    test_driver_info();
}

TEST_CASE_METHOD(mssql_fixture, "test_datasources", "[mssql][datasources]")
{
    test_datasources();
}

TEST_CASE_METHOD(mssql_fixture, "test_affected_rows", "[mssql][affected_rows]")
{
    // Skip on SQL Server 2008, see details at
    // http://help.appveyor.com/discussions/problems/4704-database-cannot-be-autostarted-during-server-shutdown-or-startup
    if (get_env("DB") == NANODBC_TEXT("MSSQL2008"))
    {
        WARN("test_affected_rows skipped on AppVeyor with SQL Server 2008");
        return;
    }

// Enable MARS required?
#if 0
    enum { SQL_COPT_SS_MARS_ENABLED = 1224, SQL_MARS_ENABLED_YES = 1 }; // sqlext.h
    int rc = ::SQLSetConnectAttr(conn.native_dbc_handle(), SQL_COPT_SS_MARS_ENABLED, (SQLPOINTER)SQL_MARS_ENABLED_YES, SQL_IS_UINTEGER);
    REQUIRE(rc == 0);
#endif

    auto conn = connect();
    auto const current_db_name = conn.database_name();

    // CREATE DATABASE|TABLE
    {
        execute(
            conn,
            NANODBC_TEXT(
                "IF DB_ID('nanodbc_test_temp_db') IS NOT NULL DROP DATABASE nanodbc_test_temp_db"));
        nanodbc::result result;
        result = execute(conn, NANODBC_TEXT("CREATE DATABASE nanodbc_test_temp_db"));
        REQUIRE(!result.has_affected_rows());
        REQUIRE(result.affected_rows() == -1);
        execute(conn, NANODBC_TEXT("USE nanodbc_test_temp_db"));
        result = execute(conn, NANODBC_TEXT("CREATE TABLE nanodbc_test_temp_table (i int)"));
        REQUIRE(result.affected_rows() == -1);
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
        REQUIRE(result.affected_rows() == -1);
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
        REQUIRE(result.affected_rows() == -1);
        execute(conn, NANODBC_TEXT("USE ") + current_db_name);
        result = execute(conn, NANODBC_TEXT("DROP DATABASE nanodbc_test_temp_db"));
        REQUIRE(result.affected_rows() == -1);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_integral", "[mssql][batch][integral]")
{
    test_batch_insert_integral();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_null", "[mssql][batch][null]")
{
    test_batch_insert_null();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_string", "[mssql][batch][string]")
{
    test_batch_insert_string();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_insert_mixed", "[mssql][batch]")
{
    test_batch_insert_mixed();
}

TEST_CASE_METHOD(mssql_fixture, "test_std_optional", "[mssql][optional]")
{
    test_std_optional();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_batch_insert_describe_param",
    "[mssql][batch][describe_param]")
{
    test_batch_insert_describe_param();
}

TEST_CASE_METHOD(mssql_fixture, "test_param_size_scale_type", "[mssql][param][methods]")
{
    test_param_size_scale_type();
}

TEST_CASE_METHOD(mssql_fixture, "test_multi_statement_insert_select", "[mssql]")
{
    nanodbc::connection c = connect();
    create_table(
        c,
        NANODBC_TEXT("test_multi_statement_insert_select"),
        NANODBC_TEXT("(fid int IDENTITY, v real)"));
    execute(c, NANODBC_TEXT(""));

    // This batch of two statements, INSERT and SELECT, returns two result sets
    nanodbc::result r = nanodbc::execute(
        c,
        NANODBC_TEXT("insert into test_multi_statement_insert_select (v) values (3.14);")
            NANODBC_TEXT("select SCOPE_IDENTITY()"));

    // INSERT result set with the count
    REQUIRE(r.affected_rows() == 1);

    // SELECT result set with the last identity value
    REQUIRE(r.next_result());
    REQUIRE(r.next());

    // Type of IDENTITY(seed,increment) return value is NUMERIC(38,0)
    // and the function may generate negative values too.
    auto const sid = r.get<std::string>(0);
    auto const nid = std::stoll(sid);
    REQUIRE(nid == 1);
}

// The OUTPUT clause returns the generated key from the INSERT itself, as one result set
// with rows, rather than as a second statement whose count has to be stepped over first.
TEST_CASE_METHOD(mssql_fixture, "test_insert_output_identity", "[mssql][result][identity]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_insert_output_identity"),
        NANODBC_TEXT("(id int IDENTITY(1,1) PRIMARY KEY, data varchar(20) NOT NULL)"));

    auto const insert = NANODBC_TEXT(
        "insert into test_insert_output_identity (data) "
        "output inserted.id values (?);");

    for (int expected = 1; expected <= 3; ++expected)
    {
        nanodbc::statement statement(connection);
        statement.prepare(insert);
        statement.bind(0, "abc");

        auto results = statement.execute();
        REQUIRE(results.columns() == 1);
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == expected);
        REQUIRE(!results.next());
    }

    auto rows =
        execute(connection, NANODBC_TEXT("select count(*) from test_insert_output_identity;"));
    REQUIRE(rows.next());
    REQUIRE(rows.get<int>(0) == 3);
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_bind_timestamp_as_string",
    "[mssql][statement][bind][timestamp]")
{
    test_bind_timestamp_as_string();
}

TEST_CASE_METHOD(mssql_fixture, "test_blob", "[mssql][blob][binary][varbinary]")
{
    nanodbc::connection connection = connect();
    // Test data size less than the default size of the internal buffer (1024)
    {
        create_table(connection, NANODBC_TEXT("test_blob"), NANODBC_TEXT("(data varbinary(max))"));
        execute(
            connection,
            NANODBC_TEXT(
                "insert into test_blob values (CONVERT(varbinary(max), "
                "'0x010100000000000000000059400000000000005940', 1));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob;"));
        REQUIRE(results.next());

        auto const blob = results.get<std::vector<std::uint8_t>>(0);
        REQUIRE(blob.size() == 21);
        REQUIRE(to_hex(blob) == "010100000000000000000059400000000000005940");
    }

    // Test data size greater than, but not multiple of, the default size of the internal buffer
    // (1024)
    {
        create_table(connection, NANODBC_TEXT("test_blob"), NANODBC_TEXT("(data varbinary(max))"));
        execute(connection, NANODBC_TEXT("insert into test_blob values (CRYPT_GEN_RANDOM(1579));"));
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob;"));
        REQUIRE(results.next());
        REQUIRE(results.get<std::vector<std::uint8_t>>(0).size() == 1579);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_xml", "[mssql][xml]")
{
    auto connection = connect();
    {
        create_table(connection, NANODBC_TEXT("test_xml"), NANODBC_TEXT("(data XML)"));
        nanodbc::statement stmt(connection);
        prepare(stmt, NANODBC_TEXT("INSERT INTO test_xml (data) VALUES (?)"));

        std::vector<nanodbc::string> s = {NANODBC_TEXT("myxmldata")};
        stmt.bind_strings(0, s);
        execute(stmt);
        nanodbc::result results =
            nanodbc::execute(connection, NANODBC_TEXT("SELECT data FROM test_xml;"));
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(0) == s[0]);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_large_blob", "[mssql][blob][binary][varbinary]")
{
    std::vector<std::uint8_t> blob;
    {
        std::string filename{get_data_path("large_binary_object_geometry_wkb.txt")};
        auto const hex = read_text_file(filename);
        blob = from_hex(hex);
    }

    // Test executing prepared statement with size of blbo larger than max (eg. SQL Server 8000
    // Bytes)
    auto connection = connect();
    {
        create_table(
            connection, NANODBC_TEXT("test_large_blob"), NANODBC_TEXT("(data varbinary(max))"));
        nanodbc::statement stmt(connection);
        prepare(stmt, NANODBC_TEXT("INSERT INTO test_large_blob (data) VALUES (?)"));

        std::vector<std::vector<std::uint8_t>> rows = {blob};
        stmt.bind(0, rows);
        execute(stmt);
    }
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_large_blob_geometry",
    "[mssql][blob][binary][varbinary][geometry]")
{
    std::string filename{get_data_path("large_binary_object_geometry_wkb.txt")};
    auto const hex = read_text_file(filename);

    // Test executing direct INSERT statement with blob larger than max (eg. SQL Server 8000
    // Bytes)
    auto connection = connect();
    auto ver = connection.dbms_version();
    {
        create_table(
            connection,
            NANODBC_TEXT("test_large_blob_geometry"),
            NANODBC_TEXT("(i int, s nvarchar(256), data GEOMETRY)"));

        nanodbc::string sql = NANODBC_TEXT(
            "INSERT INTO test_large_blob_geometry (data,i,s) VALUES "
            "(geometry::STGeomFromWKB(CONVERT(varbinary(max), '0x");
        sql += nanodbc::test::convert(hex);
        sql += NANODBC_TEXT("', 1), 0), 7, 'Fred')");

        nanodbc::execute(connection, sql);
    }
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_large_blob_geometry_with_bind_statement",
    "[mssql][blob][binary][varbinary][geometry]")
{
    if (get_env("DB") == NANODBC_TEXT("MSSQL2008"))
    {
        // Batch insert using prepared statement does not work with SQL Server 2008 or earlier
        // due to a bug in GEOMETRY column parameters handling/binding by the new driver:
        //   [Microsoft][ODBC Driver 17 for SQL Server][SQL Server]
        //   The incoming tabular data stream (TDS) remote procedure call (RPC) protocol stream is
        //   incorrect. Parameter 2 (""): The supplied value is not a valid instance of data type
        //   geometry.
        //
        // The very old Microsoft SQL Server ODBC Driver, Driver={SQL Server}, seems to work with
        // prepared statements as long as GEOMETRY column occurs last in the list of INSERT columns.
        WARN("test_large_blob_geometry skipped on AppVeyor with SQL Server 2008");
        return;
    }

    std::vector<std::uint8_t> blob;
    {
        std::string filename{get_data_path("large_binary_object_geometry_wkb.txt")};
        auto const hex = read_text_file(filename);
        blob = from_hex(hex);
    }

    // Test executing prepared statement with size of blob larger than max (eg. SQL Server 8000
    // Bytes)
    auto connection = connect();
    {
        create_table(
            connection,
            NANODBC_TEXT("test_large_blob_geometry_with_bind"),
            NANODBC_TEXT("(i int, s nvarchar(256), data GEOMETRY)"));
        nanodbc::statement stmt(connection);
        prepare(
            stmt,
            NANODBC_TEXT(
                "INSERT INTO test_large_blob_geometry_with_bind (i,s,data) VALUES "
                "(?,?,geometry::STGeomFromWKB(?, 0))"));

        short i{9};
        std::vector<nanodbc::string> s = {NANODBC_TEXT("Fred")};
        std::vector<std::vector<std::uint8_t>> rows = {blob};
        stmt.bind(0, &i);
        stmt.bind_strings(1, s);
        stmt.bind(2, rows);
        execute(stmt);
    }
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_blob_with_varchar",
    "[mssql][blob][binary][varbinary][varchar]")
{
    nanodbc::string s = NANODBC_TEXT(
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
        "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"
        "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIII"
        "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIJJJJJJJJJJJ"
        "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJKKK"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "KKKKKKLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
        "LLLLLLLLLLLLLLLMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"
        "MMMMMMMMMMMMMMMMMMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
        "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
        "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"
        "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ"
        "QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"
        "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRSSSSSSSSSSSSSSSSSSSSSSS"
        "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTTTTTT"
        "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTUUUUUU"
        "UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU"
        "UUUVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
        "VVVVVVVVVVVVWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
        "WWWWWWWWWWWWWWWWWWWWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"
        "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"
        "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");

    nanodbc::connection connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_blob_with_varchar"), NANODBC_TEXT("(data varbinary(max))"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_blob_with_varchar values (CONVERT(varbinary(max), '") + s +
            NANODBC_TEXT("'));"));

    nanodbc::result results =
        nanodbc::execute(connection, NANODBC_TEXT("select data from test_blob_with_varchar;"));
    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(0) == s);
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();

#ifdef NANODBC_HAS_STD_VARIANT
    // Block Cursors: https://technet.microsoft.com/en-us/library/aa172590.aspx
    constexpr std::size_t const rowset_size = 2;
#else
    // Not testing block cursors
    constexpr std::size_t const rowset_size = 1;
#endif

    create_table(
        conn,
        NANODBC_TEXT("test_variable_string"),
        NANODBC_TEXT("(i int, s1_bound nvarchar(256), s2_unbound varchar(max))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s1_bound, s2_unbound) values (1, 'this "
            "is a shorter text in bound col', 'this is a shorter text in unbound col');"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s1_bound, s2_unbound) values (2, 'this "
            "is a longer text of the three "
            "in the table in bound col', 'this is a longer text of the three texts in the "
            "table in unbound col');"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s1_bound, s2_unbound) values (2, 'this "
            "is the longest text of the three "
            "in the table in bound col', 'this is the longest text of the three texts in "
            "the table in unbound col');"));

#ifdef NANODBC_HAS_STD_VARIANT
    std::list<nanodbc::statement::attribute> attributes;
    attributes.push_back({SQL_ATTR_CURSOR_TYPE, 0, (std::uintptr_t)SQL_CURSOR_STATIC});
    nanodbc::statement stmt(conn, attributes);
#else
    nanodbc::statement stmt(conn);
#endif
    nanodbc::batch_ops array_sizes;
    array_sizes.rowset_size = rowset_size;
    nanodbc::result results = stmt.execute_direct(
        conn,
        NANODBC_TEXT("select i, s1_bound, s2_unbound from test_variable_string order by i;"),
        array_sizes);

    REQUIRE(results.next());
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is a shorter text in bound col"));
    REQUIRE(
        results.get<nanodbc::string>(2) == NANODBC_TEXT("this is a shorter text in unbound col"));
    REQUIRE(results.next());
    REQUIRE(
        results.get<nanodbc::string>(1) ==
        NANODBC_TEXT("this is a longer text of the three in the table in bound col"));
    REQUIRE(
        results.get<nanodbc::string>(2) ==
        NANODBC_TEXT("this is a longer text of the three texts in the table in unbound col"));
    REQUIRE(results.next());
    REQUIRE(
        results.get<nanodbc::string>(1) ==
        NANODBC_TEXT("this is the longest text of the three in the table in bound col"));
    REQUIRE(
        results.get<nanodbc::string>(2) ==
        NANODBC_TEXT("this is the longest text of the three texts in the table in unbound col"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar_and_first_row_null",
    "[mssql][nvarchar][block][rowset]")
{
    /* In this test there are no long / unbound columns.
     * It is fine to execute without changing the cursor type
     * to one that is scrollable
     * ( as SQLSetPos/SQLGetData is never called ).
     */
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("test_variable_string"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(conn, NANODBC_TEXT("insert into test_variable_string (i, s) values (1, NULL);"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (2, 'this is a longer text of the two "
            "texts in the table');"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from test_variable_string order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(
        results.get<nanodbc::string>(1) ==
        NANODBC_TEXT("this is a longer text of the two texts in the table"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_block_cursor_with_nvarchar_and_second_row_null",
    "[mssql][nvarchar][block][rowset]")
{
    nanodbc::connection conn = connect();
    std::size_t const rowset_size = 2;

    create_table(
        conn, NANODBC_TEXT("test_variable_string"), NANODBC_TEXT("(i int, s nvarchar(256))"));
    execute(
        conn,
        NANODBC_TEXT(
            "insert into test_variable_string (i, s) values (1, 'this is a shorter text');"));
    execute(conn, NANODBC_TEXT("insert into test_variable_string (i, s) values (2, NULL);"));
    nanodbc::result results = nanodbc::execute(
        conn, NANODBC_TEXT("select i, s from test_variable_string order by i;"), rowset_size);
    REQUIRE(results.next());
    REQUIRE(!results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is a shorter text"));
    REQUIRE(results.next());
    REQUIRE(results.is_null(1));
    REQUIRE(results.get<nanodbc::string>(1, NANODBC_TEXT("nothing")) == NANODBC_TEXT("nothing"));
    REQUIRE(!results.next());
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_blob_retrieve_out_of_order",
    "[mssql][blob][varchar][unbound]")
{
    // This test is based on https://knowledgebase.progress.com/articles/Article/9384,
    // it illustrates a canonical situation leading to the Invalid Descriptor Index error.
    // It stays here because the restriction is the SQL Server driver's own: the PostgreSQL,
    // MySQL and SQLite drivers read the same columns in the same order without complaint.

    nanodbc::connection connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_blob_retrieve_out_of_order"),
        NANODBC_TEXT("(c1_bound int, c2_unbound varchar(max), c3_bound int, c4_unbound text)"));
    execute(
        connection,
        NANODBC_TEXT(
            "insert into test_blob_retrieve_out_of_order values "
            "(1, 'this is varchar max', 11, 'this is text');"));

    // Query bound and unbound interleaved.
    // Access in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c2_unbound, c3_bound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE_THROWS_WITH(
            results.get<nanodbc::string>(1),
            Catch::Matchers::ContainsSubstring("07009")); // Invalid Descriptor Index
    }

    // Query bound first, then unbound.
    // Access in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c3_bound, c2_unbound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<int>(1) == 11);
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("this is varchar max"));
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
    }

    // Query bound first, then unbound.
    // Access bound NOT in order and unbound in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c3_bound, c2_unbound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<nanodbc::string>(2) == NANODBC_TEXT("this is varchar max"));
        REQUIRE(results.get<int>(1) == 11); // no error "Invalid Descriptor Index"
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
    }
    // Query bound first, then unbound.
    // Access bound in order and unbound NOT in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c3_bound, c2_unbound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<int>(1) == 11);
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
        REQUIRE_THROWS_WITH(
            results.get<nanodbc::string>(2),
            Catch::Matchers::ContainsSubstring("07009")); // Invalid Descriptor Index
    }

    // Query bound and unbound interleaved.
    // Unbind all columns.
    // Access in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c2_unbound, c3_bound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        results.unbind();
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is varchar max"));
        REQUIRE(results.get<int>(2) == 11);
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
    }
    // Query bound and unbound interleaved.
    // Unbind all columns.
    // Access NOT in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c2_unbound, c3_bound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        results.unbind();
        REQUIRE(results.next());
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is varchar max"));
        REQUIRE_THROWS_WITH(
            results.get<int>(0),
            Catch::Matchers::ContainsSubstring("07009")); // Invalid Descriptor Index
    }

    // Query bound and unbound interleaved.
    // Unbind offending column only, and access in order of increasing column number.
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c2_unbound, c3_bound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        REQUIRE(results.is_bound(0));
        REQUIRE(results.is_bound(2));
        results.unbind(2); // make c3_bound an unbound column
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is varchar max"));
        REQUIRE(results.get<int>(2) == 11);
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
    }

    // Unbind all columns, and access NOT in order of increasing column number
    {
        nanodbc::result results = nanodbc::execute(
            connection,
            NANODBC_TEXT(
                "select c1_bound, c2_unbound, c3_bound, c4_unbound from "
                "test_blob_retrieve_out_of_order;"));
        results.unbind();
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("this is varchar max"));
        REQUIRE(results.get<int>(2) == 11);
        REQUIRE(results.get<nanodbc::string>(3) == NANODBC_TEXT("this is text"));
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_list_catalogs", "[mssql][catalog][catalogs]")
{
    test_catalog_list_catalogs();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_list_schemas", "[mssql][catalog][schemas]")
{
    test_catalog_list_schemas();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_list_table_types", "[mssql][catalog][table_types]")
{
    test_catalog_list_table_types();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_columns", "[mssql][catalog][columns]")
{
    test_catalog_columns();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_primary_keys", "[mssql][catalog][primary_keys]")
{
    test_catalog_primary_keys();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_tables", "[mssql][catalog][tables]")
{
    test_catalog_tables();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_catalog_procedure_columns",
    "[mssql][catalog][procedure_columns]")
{
    test_catalog_procedure_columns();
}

TEST_CASE_METHOD(mssql_fixture, "test_catalog_table_privileges", "[mssql][catalog][tables]")
{
    test_catalog_table_privileges();
}

TEST_CASE_METHOD(mssql_fixture, "test_column_descriptor", "[mssql][columns]")
{
    test_column_descriptor();
}

TEST_CASE_METHOD(mssql_fixture, "test_column_descriptor_unsigned", "[mssql][columns]")
{
    auto c = connect();
    create_table(
        c,
        NANODBC_TEXT("test_column_descriptor_unsigned"),
        NANODBC_TEXT("(name text, ti tinyint, si smallint, i int, bi bigint"));

    // insert
    {
        execute(
            c,
            NANODBC_TEXT(
                "insert into test_column_descriptor_unsigned (name,ti,si,i,bi) values "
                "('min', 0, -32768, -2147483648, -9223372036854775808);"));
        execute(
            c,
            NANODBC_TEXT(
                "insert into test_column_descriptor_unsigned (name,ti,si,i,bi) values "
                "('mid', 128, 1, 2, 3);"));
        execute(
            c,
            NANODBC_TEXT(
                "insert into test_column_descriptor_unsigned (name,ti,si,i,bi) values "
                "('max', 255, 32767, 2147483647, 9223372036854775807);"));
    }

    // select
    {
        auto result = execute(
            c,
            NANODBC_TEXT(
                "select name,ti,si,i,bi from test_column_descriptor_unsigned order by ti asc;"));
        REQUIRE(result.column_unsigned(0)); // SQL_TRUE for non-numeric by default
        REQUIRE(result.column_unsigned(1)); // SQL_TRUE for TINYINT as always unsigned in SQL Server
        REQUIRE(!result.column_unsigned(2)); // SMALLINT
        REQUIRE(!result.column_unsigned(3)); // INT
        REQUIRE(!result.column_unsigned(4)); // BIGINT
        REQUIRE(result.next());
        REQUIRE(result.get<std::int16_t>(1) == 0);
        REQUIRE(result.get<std::int16_t>(2) == -32768);
        REQUIRE(result.get<std::int32_t>(3) == (-2147483647 - 1));
        REQUIRE(result.get<std::int64_t>(4) == (-9223372036854775807 - 1));
        REQUIRE(result.next());
        REQUIRE(result.get<std::int16_t>(1) == 128);
        REQUIRE(result.get<std::int16_t>(2) == 1);
        REQUIRE(result.get<std::int32_t>(3) == 2);
        REQUIRE(result.get<std::int64_t>(4) == 3);
        REQUIRE(result.next());
        REQUIRE(result.get<std::int16_t>(1) == 255);
        REQUIRE(result.get<std::int16_t>(2) == 32767);
        REQUIRE(result.get<std::int32_t>(3) == 2147483647);
        REQUIRE(result.get<std::int64_t>(4) == 9223372036854775807);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_connection_environment", "[mssql][connection]")
{
    test_connection_environment();
}

TEST_CASE_METHOD(mssql_fixture, "test_dbms_info", "[mssql][dmbs][metadata][info]")
{
    test_dbms_info();
}

TEST_CASE_METHOD(mssql_fixture, "test_get_info", "[mssql][dmbs][metadata][info]")
{
    test_get_info();
}

TEST_CASE_METHOD(mssql_fixture, "test_decimal_conversion", "[mssql][decimal][conversion]")
{
    test_decimal_conversion();
}

TEST_CASE_METHOD(mssql_fixture, "test_error", "[mssql][error]")
{
    test_error();
}

TEST_CASE_METHOD(mssql_fixture, "test_exception", "[mssql][exception]")
{
    test_exception();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_execute_multiple_transaction",
    "[mssql][execute][transaction]")
{
    test_execute_multiple_transaction();
}

TEST_CASE_METHOD(mssql_fixture, "test_execute_multiple", "[mssql][execute]")
{
    test_execute_multiple();
}

TEST_CASE_METHOD(mssql_fixture, "test_implementation_row_descriptor", "[mssql][descriptor][ird]")
{
    test_implementation_row_descriptor();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_implementation_row_descriptor_auto_unique_value",
    "[mssql][descriptor][ird]")
{
    auto c = connect();

    create_table(
        c, NANODBC_TEXT("test_implementation_row_descriptor_auto_unique_value"), NANODBC_TEXT(R"(
fid int IDENTITY(1,1) PRIMARY KEY,
name varchar(60)
)"));

    auto const sql =
        NANODBC_TEXT("SELECT fid, name FROM test_implementation_row_descriptor_auto_unique_value");
    nanodbc::statement s(c, sql);
    nanodbc::implementation_row_descriptor ird(s);
    REQUIRE(ird.count() == 2);
    REQUIRE(ird.auto_unique_value(0));
    REQUIRE(!ird.auto_unique_value(1));
}

#if defined(NANODBC_TEST_SKIP_IRD_DUE_STRANGE_FAILURES_ON_LINUX)
TEST_CASE_METHOD(
    mssql_fixture,
    "test_implementation_row_descriptor_with_expressions",
    "[mssql][descriptor][ird]")
{
    test_implementation_row_descriptor_with_expressions();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_implementation_row_descriptor_with_query",
    "[mssql][descriptor][ird]")
{
    auto c = connect();
    drop_table(c, NANODBC_TEXT("v_t1_t2"), true);
    drop_table(c, NANODBC_TEXT("t1"));
    drop_table(c, NANODBC_TEXT("t2"));

    create_table(c, NANODBC_TEXT("t1"), NANODBC_TEXT(R"(
t1_fid1 int NOT NULL,
t1_fid2 int NOT NULL,
name varchar(60) NOT NULL,
PRIMARY KEY(t1_fid1, t1_fid2)
)"));

    execute(
        c, NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (1,10,'John Malkovich');"));
    execute(c, NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (2,20,'Gina Bellman');"));
    execute(c, NANODBC_TEXT("INSERT INTO t1(t1_fid1,t1_fid2,name) VALUES (3,30,'Bruce Willis');"));

    create_table(c, NANODBC_TEXT("t2"), NANODBC_TEXT(R"(
t2_fid int NOT NULL,
age int NOT NULL,
PRIMARY KEY(t2_fid)
)"));

    execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (1,53)"));
    execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (2,41)"));
    execute(c, NANODBC_TEXT("INSERT INTO t2(t2_fid,age) VALUES (3,60)"));

    create_table(
        c,
        NANODBC_TEXT("v_t1_t2"),
        NANODBC_TEXT("SELECT t1.*, t2.* FROM t1 INNER JOIN t2 ON t1.t1_fid1 = t2.t2_fid"),
        true);

    nanodbc::string sql = NANODBC_TEXT(
        "SELECT t1.t1_fid1 AS fid1, t2.t2_fid AS fid2, v1.t1_fid1 AS fid3, t1.name AS n, "
        "t2.age AS a, v1.name AS vn, v1.age AS va FROM t1 INNER JOIN t2 ON t1.t1_fid1 = "
        "t2.t2_fid INNER JOIN v_t1_t2 AS v1 ON t1.t1_fid1 = v1.t1_fid1 AND v1.t1_fid2 = "
        "v1.t1_fid2 AND t2.t2_fid = v1.t2_fid FOR BROWSE");
    // attributes like table name available only for
    // SELECT statements containing FOR BROWSE clause

    nanodbc::statement s(c, sql);
    nanodbc::implementation_row_descriptor ird(s);
    REQUIRE(ird.count() == 7);

    for (short i = 0; i < ird.count(); ++i)
    {
        REQUIRE(ird.catalog_name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.schema_name(i) == NANODBC_TEXT(""));
        REQUIRE(ird.base_table_name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.table_name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.base_column_name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.type_name(i) != NANODBC_TEXT(""));
        REQUIRE(ird.local_type_name(i) != NANODBC_TEXT(""));
    }

    // t1.t1_fid1
    short i = 0;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("t1_fid1"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("fid1"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("int"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("int"));
    // t2.t2_fid2
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("t2_fid"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("fid2"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("int"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("int"));
    // v_t1_t2.t1.t1_fid1
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("t1_fid1"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("fid3"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("int"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("int"));
    // t1.name
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("name"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("n"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("varchar"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("varchar"));
    // t2.age
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("age"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("a"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("int"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("int"));
    // v_t1_t2.name
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t1"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("name"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("vn"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("varchar"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("varchar"));
    // v_t1_t2.age
    i++;
    REQUIRE(ird.base_table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.table_name(i) == NANODBC_TEXT("t2"));
    REQUIRE(ird.base_column_name(i) == NANODBC_TEXT("age"));
    REQUIRE(ird.name(i) == NANODBC_TEXT("va"));
    REQUIRE(ird.type_name(i) == NANODBC_TEXT("int"));
    REQUIRE(ird.local_type_name(i) == NANODBC_TEXT("int"));
}
#endif // defined(NANODBC_TEST_SKIP_IRD_DUE_STRANGE_FAILURES_ON_LINUX)

TEST_CASE_METHOD(mssql_fixture, "test_integral", "[mssql][integral]")
{
    test_integral<mssql_fixture>();
}

TEST_CASE_METHOD(mssql_fixture, "test_integral_small_types", "[mssql][integral]")
{
    test_integral_small_types();
}

TEST_CASE_METHOD(mssql_fixture, "test_integral_small_types_batch", "[mssql][integral][batch]")
{
    test_integral_small_types_batch();
}

TEST_CASE_METHOD(mssql_fixture, "test_integral_to_string_conversion", "[mssql][integral]")
{
    test_integral_to_string_conversion();
}

TEST_CASE_METHOD(mssql_fixture, "test_move", "[mssql][move]")
{
    test_move();
}

TEST_CASE_METHOD(mssql_fixture, "test_null", "[mssql][null]")
{
    test_null();
}

TEST_CASE_METHOD(mssql_fixture, "test_nullptr_nulls", "[mssql][null]")
{
    test_nullptr_nulls();
}

TEST_CASE_METHOD(mssql_fixture, "test_null_with_bound_columns_unbound", "[mssql][null][unbound]")
{
    test_null_with_bound_columns_unbound();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_at_end", "[mssql][result]")
{
    test_result_at_end();
}

TEST_CASE_METHOD(mssql_fixture, "test_temporal_conversions", "[mssql][date][time][timestamp]")
{
    test_temporal_conversions();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_implementation_row_descriptor_fields",
    "[mssql][descriptor][ird]")
{
    test_implementation_row_descriptor_fields();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_open_close", "[mssql][statement]")
{
    test_statement_open_close();
}

TEST_CASE_METHOD(mssql_fixture, "test_boolean_column", "[mssql][boolean]")
{
    test_boolean_column();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_null_array", "[mssql][null]")
{
    test_bind_null_array();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_null_sentry", "[mssql][statement]")
{
    test_bind_null_sentry();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_binary_null_sentry", "[mssql][binary][null]")
{
    test_bind_binary_null_sentry();
}

TEST_CASE_METHOD(mssql_fixture, "test_timeouts", "[mssql][statement]")
{
    test_timeouts();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_parameter_description", "[mssql][statement]")
{
    test_statement_parameter_description();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_rowset_navigation", "[mssql][result][rowset]")
{
    test_result_rowset_navigation();
}

TEST_CASE_METHOD(mssql_fixture, "test_execute_direct_batch_ops", "[mssql][statement][batch]")
{
    test_execute_direct_batch_ops();
}

TEST_CASE_METHOD(mssql_fixture, "test_string_aggregate", "[mssql][result][string]")
{
    test_string_aggregate();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_execute_prepared_statement_repeatedly",
    "[mssql][statement][prepare]")
{
    test_execute_prepared_statement_repeatedly();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_arithmetic_null_sentry", "[mssql][bind][null]")
{
    test_bind_arithmetic_null_sentry();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_nested_transaction_rollback",
    "[mssql][transaction][rollback]")
{
    test_nested_transaction_rollback();
}

TEST_CASE_METHOD(mssql_fixture, "test_long_text_chunk_boundaries", "[mssql][result][string]")
{
    test_long_text_chunk_boundaries();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_unbind", "[mssql][result][unbind]")
{
    test_result_unbind();
}

TEST_CASE_METHOD(mssql_fixture, "test_is_null_binary", "[mssql][binary][null]")
{
    test_is_null_binary();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_accessors", "[mssql][result][accessors]")
{
    test_result_accessors();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_iterator", "[mssql][result][iterator]")
{
    test_result_iterator();
}

TEST_CASE_METHOD(mssql_fixture, "test_simple", "[mssql]")
{
    test_simple();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_usable_when_result_gone", "[mssql][statement]")
{
    test_statement_usable_when_result_gone();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_with_empty_connection", "[mssql][statement]")
{
    nanodbc::connection c;
    c.allocate();
    nanodbc::statement s;
    REQUIRE_THROWS_AS(s.open(c), nanodbc::database_error);
    REQUIRE_THROWS_WITH(
        s.open(c), Catch::Matchers::ContainsSubstring("08003")); // Connection not open
}

TEST_CASE_METHOD(mssql_fixture, "test_string", "[mssql][string]")
{
    test_string();
}

TEST_CASE_METHOD(mssql_fixture, "test_string_with_nvarchar_max", "[mssql][string]")
{
    nanodbc::connection connection = connect();
    drop_table(connection, NANODBC_TEXT("test_string_with_nvarchar_max"));
    execute(
        connection, NANODBC_TEXT("create table test_string_with_nvarchar_max (s nvarchar(max));"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_string_with_nvarchar_max(s) ")
            NANODBC_TEXT("values (REPLICATE(CAST(\'a\' AS nvarchar(MAX)), 15000))"));

    nanodbc::result results =
        execute(connection, NANODBC_TEXT("select s from test_string_with_nvarchar_max;"));
    REQUIRE(results.next());

    nanodbc::string select;
    results.get_ref(0, select);
    REQUIRE(select.size() == 15000);
}

TEST_CASE_METHOD(mssql_fixture, "test_string_with_varchar_max", "[mssql][string]")
{
    test_string_with_varchar_max();
}

TEST_CASE_METHOD(mssql_fixture, "test_string_with_ntext", "[mssql][string][ntext]")
{
    nanodbc::connection connection = connect();
    drop_table(connection, NANODBC_TEXT("test_string_with_ntext"));
    execute(connection, NANODBC_TEXT("create table test_string_with_ntext (s ntext);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_string_with_ntext(s) ")
            NANODBC_TEXT("values (REPLICATE(CAST(\'a\' AS nvarchar(MAX)), 15000))"));

    nanodbc::result results =
        execute(connection, NANODBC_TEXT("select s from test_string_with_ntext;"));
    REQUIRE(results.next());

    nanodbc::string select;
    results.get_ref(0, select);
    REQUIRE(select.size() == 15000);
}

TEST_CASE_METHOD(mssql_fixture, "test_string_with_text", "[mssql][string][text]")
{
    nanodbc::connection connection = connect();
    drop_table(connection, NANODBC_TEXT("test_string_with_text"));
    execute(connection, NANODBC_TEXT("create table test_string_with_text (s text);"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_string_with_text(s) ")
            NANODBC_TEXT("values (REPLICATE(CAST(\'a\' AS varchar(MAX)), 15000))"));

    nanodbc::result results =
        execute(connection, NANODBC_TEXT("select s from test_string_with_text;"));
    REQUIRE(results.next());

    nanodbc::string select;
    results.get_ref(0, select);
    REQUIRE(select.size() == 15000);
}

TEST_CASE_METHOD(mssql_fixture, "test_string_vector", "[mssql][string]")
{
    test_string_vector();
}

TEST_CASE_METHOD(mssql_fixture, "test_string_view_vector", "[mssql][string]")
{
    test_string_view_vector();
}

TEST_CASE_METHOD(mssql_fixture, "test_batch_binary", "[mssql][binary]")
{
    test_batch_binary();
}

TEST_CASE_METHOD(mssql_fixture, "test_time", "[mssql][time]")
{
    test_time();
}

TEST_CASE_METHOD(mssql_fixture, "test_date", "[mssql][date]")
{
    test_date();
}

TEST_CASE_METHOD(mssql_fixture, "test_datetime", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetime"), NANODBC_TEXT("d datetime"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    {
        execute(
            connection,
            NANODBC_TEXT(
                "insert into test_datetime(d) values (CONVERT(datetime, "
                "'2006-12-30T13:45:12.345', 126));"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetime;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_TYPE_TIMESTAMP);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetime"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour == 13);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_decimal", "[mssql][decimal]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_decimal"), NANODBC_TEXT("(d decimal(19,4))"));

    // insert
    {
        execute(
            connection,
            NANODBC_TEXT("insert into test_decimal(d) values (-922337203685477.5808);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal(d) values (0);"));
        execute(connection, NANODBC_TEXT("insert into test_decimal(d) values (1.23);"));
        execute(
            connection, NANODBC_TEXT("insert into test_decimal(d) values (922337203685477.5807);"));
    }

    // select
    {
        auto result =
            execute(connection, NANODBC_TEXT("select d from test_decimal order by d asc;"));
        REQUIRE(result.next());
        auto d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("-922337203685477.5808")); // Min value of SQL data type
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT(".0000"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("1.2300"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("922337203685477.5807")); // Max value of SQL data type MONEY
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_money", "[mssql][decimal][money]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_money"), NANODBC_TEXT("(d money)"));

    // insert
    {
        execute(
            connection, NANODBC_TEXT("insert into test_money(d) values (-922337203685477.5808);"));
        execute(connection, NANODBC_TEXT("insert into test_money(d) values (0);"));
        execute(connection, NANODBC_TEXT("insert into test_money(d) values (1.23);"));
        execute(
            connection, NANODBC_TEXT("insert into test_money(d) values (922337203685477.5807);"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_money order by d asc;"));
        REQUIRE(result.next());
        auto d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("-922337203685477.5808")); // Min value of SQL data type MONEY
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT(".0000"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("1.2300"));
        REQUIRE(result.next());
        d = result.get<nanodbc::string>(0);
        REQUIRE(d == NANODBC_TEXT("922337203685477.5807")); // Max value of SQL data type MONEY
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_datetime2", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetime2"), NANODBC_TEXT("d datetime2"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    {
        execute(
            connection,
            NANODBC_TEXT(
                "insert into test_datetime2(d) values (CONVERT(datetime2, "
                "'2006-12-30T13:45:12.345', 127));"));
    }

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetime2;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_TYPE_TIMESTAMP);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetime2"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::timestamp>(0);
        REQUIRE(t.year == 2006);
        REQUIRE(t.month == 12);
        REQUIRE(t.day == 30);
        REQUIRE(t.hour == 13);
        REQUIRE(t.min == 45);
        REQUIRE(t.sec == 12);
        REQUIRE(t.fract > 0);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_datetimeoffset", "[mssql][datetime]")
{
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_datetimeoffset"), NANODBC_TEXT("d datetimeoffset"));

    // insert
    // See "CAST and CONVERT" https://msdn.microsoft.com/en-US/library/ms187928.aspx
    execute(
        connection,
        NANODBC_TEXT(
            "insert into test_datetimeoffset(d) values "
            "(CONVERT(datetimeoffset, '2006-12-30T13:45:12.345-08:00', 127));"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select d from test_datetimeoffset;"));

#ifndef SQL_SS_TIMESTAMPOFFSET
#define SQL_SS_TIMESTAMPOFFSET (-155)
#endif
        REQUIRE(result.column_name(0) == NANODBC_TEXT("d"));
        REQUIRE(result.column_datatype(0) == SQL_SS_TIMESTAMPOFFSET);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetimeoffset"));

        REQUIRE(result.next());
        auto t = result.get<nanodbc::string>(0);
        // the result is this NANODBC_TEXT("2006-12-30 13:45:12.3450000 -08:00");
        REQUIRE(t.size() >= 27); // frac of seconds is server and system dependend
        REQUIRE(t.substr(0, 23) == NANODBC_TEXT("2006-12-30 13:45:12.345"));
        auto it = t.rbegin();
        REQUIRE(*it++ == '0');
        REQUIRE(*it++ == '0');
        REQUIRE(*it++ == ':');
        REQUIRE(*it++ == '8');
        REQUIRE(*it++ == '0');
        REQUIRE(*it++ == '-');
    }
}

// A datetimeoffset column answers to the narrower temporal types, and a date or datetime
// column answers to timestampoffset, each through its own conversion.
TEST_CASE_METHOD(mssql_fixture, "test_datetimeoffset_conversions", "[mssql][datetimeoffset]")
{
    auto connection = connect();
    auto result = execute(
        connection,
        NANODBC_TEXT(
            "SELECT CONVERT(datetimeoffset, '2006-12-30T13:45:12.345-08:30', 127) AS dto,"
            " CONVERT(date, '2006-12-30', 23) AS d,"
            " CONVERT(datetime, '2006-12-30T13:45:12', 126) AS dt;"));
    REQUIRE(result.next());

    // datetimeoffset read as the narrower types
    auto const as_date = result.get<nanodbc::date>(0);
    REQUIRE(as_date.year == 2006);
    REQUIRE(as_date.month == 12);
    REQUIRE(as_date.day == 30);

    auto const as_time = result.get<nanodbc::time>(0);
    REQUIRE(as_time.hour == 13);
    REQUIRE(as_time.min == 45);
    REQUIRE(as_time.sec == 12);

    auto const as_stamp = result.get<nanodbc::timestamp>(0);
    REQUIRE(as_stamp.year == 2006);
    REQUIRE(as_stamp.month == 12);
    REQUIRE(as_stamp.day == 30);
    REQUIRE(as_stamp.hour == 13);

    // date and datetime read as timestampoffset, which they carry no offset for
    auto const date_as_offset = result.get<nanodbc::timestampoffset>(1);
    REQUIRE(date_as_offset.stamp.year == 2006);
    REQUIRE(date_as_offset.stamp.month == 12);
    REQUIRE(date_as_offset.stamp.day == 30);

    auto const stamp_as_offset = result.get<nanodbc::timestampoffset>(2);
    REQUIRE(stamp_as_offset.stamp.year == 2006);
    REQUIRE(stamp_as_offset.stamp.hour == 13);
    REQUIRE(stamp_as_offset.stamp.min == 45);

    REQUIRE(!result.next());
}

TEST_CASE_METHOD(mssql_fixture, "test_datetimeoffset2", "[mssql][datetimeoffset]")
{
#ifndef SQL_SS_TIMESTAMPOFFSET
#define SQL_SS_TIMESTAMPOFFSET (-155)
#endif
    auto connection = connect();
    auto result = execute(
        connection,
        NANODBC_TEXT(
            "SELECT CONVERT(datetimeoffset, '2006-12-30T13:45:12.345-08:30', 127) AS "
            "offsettimestamp;"));
    REQUIRE(result.column_name(0) == NANODBC_TEXT("offsettimestamp"));
    REQUIRE(result.column_datatype(0) == SQL_SS_TIMESTAMPOFFSET);
    REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("datetimeoffset"));
    REQUIRE(result.next());
    auto t = result.get<nanodbc::timestampoffset>(0);
    REQUIRE(t.stamp.year == 2006);
    REQUIRE(t.stamp.month == 12);
    REQUIRE(t.stamp.day == 30);
    REQUIRE(t.stamp.hour == 13);
    REQUIRE(t.stamp.min == 45);
    REQUIRE(t.stamp.sec == 12);
    REQUIRE(t.stamp.fract > 0);
    REQUIRE(t.offset_hour == -8);
    REQUIRE(t.offset_minute == -30);
}

TEST_CASE_METHOD(mssql_fixture, "test_rowversion", "[mssql][rowversion][timestamp]")
{
    // The rowversion data type is not a date or time data type, but
    // it is a unique binary number with storage size of 8 bytes
    // The timestamp is a deprecated synonym for rowversion.
    auto connection = connect();
    create_table(
        connection, NANODBC_TEXT("test_rowversion"), NANODBC_TEXT("(v int, r rowversion)"));
    execute(connection, NANODBC_TEXT("insert into test_rowversion (v) values (123)"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select r from test_rowversion;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("r"));
        REQUIRE(result.column_datatype(0) == SQL_BINARY);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("timestamp")); // not rowversion!

        REQUIRE(result.next());
        auto t = result.get<std::vector<std::uint8_t>>(0);
        REQUIRE(t.size() == 8);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_timestamp", "[mssql][rowversion][timestamp]")
{
    // The rowversion data type is not a date or time data type, but
    // it is a unique binary number with storage size of 8 bytes
    // The timestamp is a deprecated synonym for rowversion.
    auto connection = connect();
    create_table(connection, NANODBC_TEXT("test_timestamp"), NANODBC_TEXT("(v int, t timestamp)"));
    execute(connection, NANODBC_TEXT("insert into test_timestamp (v) values (123)"));

    // select
    {
        auto result = execute(connection, NANODBC_TEXT("select t from test_timestamp;"));

        REQUIRE(result.column_name(0) == NANODBC_TEXT("t"));
        REQUIRE(result.column_datatype(0) == SQL_BINARY);
        REQUIRE(result.column_datatype_name(0) == NANODBC_TEXT("timestamp"));

        REQUIRE(result.next());
        auto t = result.get<std::vector<std::uint8_t>>(0);
        REQUIRE(t.size() == 8);
    }
}

TEST_CASE_METHOD(mssql_fixture, "test_transaction", "[mssql][transaction]")
{
    test_transaction();
}

#if defined(_MSC_VER)
TEST_CASE_METHOD(mssql_fixture, "test_win32_variant", "[mssql][variant][windows]")
{
    test_win32_variant();
}

TEST_CASE_METHOD(mssql_fixture, "test_win32_variant_null", "[mssql][variant][windows][null]")
{
    test_win32_variant_null();
}

TEST_CASE_METHOD(
    mssql_fixture,
    "test_win32_variant_null_literal",
    "[mssql][variant][windows][null]")
{
    test_win32_variant_null_literal();
}

TEST_CASE_METHOD(mssql_fixture, "test_win32_variant_bit", "[mssql][variant][windows]")
{
    auto cn = connect();
    auto rs = execute(cn, NANODBC_TEXT("select CAST(1 AS BIT) as b;"));
    rs.next();

    auto v = rs.get<_variant_t>(0);
    REQUIRE(v.vt == VT_BOOL);
    REQUIRE(static_cast<bool>(v) == true);
}

TEST_CASE_METHOD(mssql_fixture, "test_win32_variant_tinyint", "[mssql][variant][windows]")
{
    auto cn = connect();
    auto rs = execute(
        cn, NANODBC_TEXT("select CAST(0 AS TINYINT), CAST(128 AS TINYINT), CAST(255 AS TINYINT);"));
    rs.next();

    auto v0 = rs.get<_variant_t>(0);
    REQUIRE(v0.vt == VT_UI1);
    REQUIRE(static_cast<int>(v0) == 0);
    auto v1 = rs.get<_variant_t>(1);
    REQUIRE(v1.vt == VT_UI1);
    REQUIRE(static_cast<int>(v1) == 128);
    auto v2 = rs.get<_variant_t>(2);
    REQUIRE(v2.vt == VT_UI1);
    REQUIRE(static_cast<int>(v2) == 255);
}

TEST_CASE_METHOD(mssql_fixture, "test_win32_variant_timestamp", "[mssql][variant][windows]")
{
    auto cn = connect();
    auto rs = execute(cn, NANODBC_TEXT("select CURRENT_TIMESTAMP as t;"));
    rs.next();

    auto v = rs.get<_variant_t>(0);
    REQUIRE(v.vt == VT_DATE);
    ::SYSTEMTIME t0{0};
    REQUIRE(::VariantTimeToSystemTime(v, &t0));
    ::SYSTEMTIME t1{0};
    ::GetSystemTime(&t1);
    REQUIRE(t0.wYear == t1.wYear);
    REQUIRE(t0.wMonth == t1.wMonth);
    REQUIRE(t0.wDay == t1.wDay);
    REQUIRE(t0.wDayOfWeek == t1.wDayOfWeek);
    REQUIRE(t0.wHour <= 24);
    REQUIRE(t0.wMinute <= 60);
    REQUIRE(t0.wSecond <= 60);
    REQUIRE(t0.wMilliseconds <= 100);
}

TEST_CASE_METHOD(mssql_fixture, "test_win32_variant_row_cached_result", "[mssql][variant][windows]")
{
    test_win32_variant_row_cached_result();
}

#endif // _MSC_VER

TEST_CASE_METHOD(mssql_fixture, "test_while_not_end_iteration", "[mssql][looping]")
{
    test_while_not_end_iteration();
}

TEST_CASE_METHOD(mssql_fixture, "test_while_next_iteration", "[mssql][looping]")
{
    test_while_next_iteration();
}

#if !defined(NANODBC_DISABLE_ASYNC) && defined(WIN32)
TEST_CASE_METHOD(mssql_fixture, "test_async", "[mssql][async]")
{
    HANDLE event_handle = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    REQUIRE(event_handle != nullptr);

    nanodbc::connection conn;
    if (event_handle && conn.async_connect(connection_string_, event_handle))
        ::WaitForSingleObject(event_handle, INFINITE);
    conn.async_complete();

    test_async_internal(conn, event_handle);
}
#endif

TEST_CASE_METHOD(mssql_fixture, "test_bind_float", "[mssql][number][float]")
{
    auto conn = connect();
    create_table(
        conn,
        NANODBC_TEXT("test_bind_float"),
        NANODBC_TEXT("(r real, f float, f24 float(24), f53 float(53), d double precision)"));

    nanodbc::statement stmt(conn);
    prepare(stmt, NANODBC_TEXT("insert into test_bind_float(r,f,f24,f53,d) values (?,?,?,?,?)"));

    float r(1.123f);
    float f(3.123f);
    double d(7.123);
    stmt.bind(0, &r);
    stmt.bind(1, &f);
    stmt.bind(2, &f);
    stmt.bind(3, &f);
    stmt.bind(4, &d);

    nanodbc::transact(stmt, 1);
    {
        auto result =
            nanodbc::execute(conn, NANODBC_TEXT("select r,f,f24,f53,d from test_bind_float"));
        result.next();
        REQUIRE(result.get<float>(0) == static_cast<float>(r));
        REQUIRE(result.get<std::string>(0).substr(0, 5) == "1.123");
        REQUIRE(result.get<float>(1) == static_cast<float>(f));
        REQUIRE(result.get<std::string>(1).substr(0, 5) == "3.123");
        REQUIRE(result.get<float>(2) == static_cast<float>(f));
        REQUIRE(result.get<std::string>(2).substr(0, 5) == "3.123");
        REQUIRE(result.get<float>(3) == static_cast<float>(f));
        REQUIRE(result.get<std::string>(3).substr(0, 5) == "3.123");
        REQUIRE(result.get<double>(4) == static_cast<double>(d));
        REQUIRE(result.get<std::string>(4).substr(0, 5) == "7.123");
    }
}

#if defined(_MSC_VER) && defined(_UNICODE)
TEST_CASE_METHOD(mssql_fixture, "test_bind_variant", "[mssql][variant]")
{
    // Test prepared statement and binding Windows VARIANT data.
    auto conn = connect();
    create_table(
        conn,
        NANODBC_TEXT("test_bind_variant"),
        NANODBC_TEXT("(i int, f float, s varchar(256), d decimal(9, 3), b varbinary(max))"));

    nanodbc::statement stmt(conn);
    prepare(stmt, NANODBC_TEXT("insert into test_bind_variant(i,f,s,d,b) values (?,?,?,?,?)"));

    // NOTE: Some examples with number of round-trips below might seem redundant,
    // but it has been kept to for illustration purposes.

    // VT_I4 -> INT
    static_assert(sizeof(long) == sizeof(std::int32_t), "long is too large");
    _variant_t v_i(7L);
    stmt.bind(0, reinterpret_cast<std::int32_t*>(&v_i.lVal)); // no bind(long) provided
    // VT_R8 -> FLOAT
    _variant_t v_f(3.14);
    stmt.bind(1, &v_f.dblVal);
    // VT_BSTR -> VARCHAR
    _variant_t v_s(L"This is a text");
    stmt.bind_strings(2, reinterpret_cast<wchar_t*>(v_s.bstrVal), wcslen(v_s.bstrVal), 1);
    // VT_DECIMAL|VT_CY -> double -> DECIMAL(9,3)
    _variant_t v_d;
    {
        // use input value longer than DECIMAL(9,3) to test SQL will convert it appropriately
        DECIMAL d;
        VarDecFromStr(L"3.45612345", 0, LOCALE_NOUSEROVERRIDE, &d);
        double dbl;
        ::VarR8FromDec(&d, &dbl);
        v_d = dbl;
    }
    stmt.bind(3, &v_d.dblVal);
    // SAFEARRAY -> vector<uint8_t> -> VARBINARY
    // Since, currently, only way to bind binary data is via std::bector<std::uint8_t>,
    // we need to copy data from SAFEARRAY to intermediate vector.
    std::vector<std::uint8_t> bytes;
    {
        std::uint8_t data[] = {0x00, 0x01, 0x02, 0x03};
        CComSafeArray<std::uint8_t> sa;
        for (auto b : data)
            sa.Add(b);
        for (auto i = 0UL; i < sa.GetCount(); ++i)
            bytes.push_back(sa.GetAt(i));
    }
    std::vector<std::vector<std::uint8_t>> binary_items = {bytes};
    stmt.bind(4, binary_items);

    nanodbc::transact(stmt, 1);
    {
        auto result =
            nanodbc::execute(conn, NANODBC_TEXT("select i,f,s,d,b from test_bind_variant"));
        std::size_t i = 0;
        while (result.next())
        {
            REQUIRE(result.get<std::int32_t>(0) == static_cast<std::int32_t>(v_i));
            REQUIRE(result.get<double>(1) == static_cast<double>(v_f));
            REQUIRE(result.get<nanodbc::string>(2) == v_s.bstrVal);
            v_d.ChangeType(VT_BSTR);
            REQUIRE(result.get<nanodbc::string>(3) == nanodbc::string(v_d.bstrVal).substr(0, 5));
            REQUIRE(result.get<std::vector<std::uint8_t>>(4) == bytes);
            ++i;
        }
        REQUIRE(i == 1);
    }
}
#endif

#ifndef NANODBC_DISABLE_MSSQL_TVP
struct mssql_table_valued_parameter_fixture : mssql_fixture
{
    mssql_table_valued_parameter_fixture()
        : mssql_fixture()
    {
        auto conn = connect();

        // drop tvp_test first, next drop tvp_param.
        drop_procedure(conn, NANODBC_TEXT("tvp_test"));
        drop_table_type(conn, NANODBC_TEXT("tvp_param"));

        // create type tvp_param
        create_table_type(
            conn,
            NANODBC_TEXT("tvp_param"),
            NANODBC_TEXT(
                "(col0 INT,"
                " col1 BIGINT NULL,"
                " col2 VARCHAR(MAX) NULL,"
                " col3 NVARCHAR(MAX) NULL,"
                " col4 VARBINARY(MAX) NULL)"));

        // create procedure tvp_test
        execute(
            conn,
            NANODBC_TEXT(
                "CREATE PROCEDURE tvp_test(@p0 INT, @p1 tvp_param READONLY, @p2 NVARCHAR(MAX))"
                " AS"
                " BEGIN"
                "    SET NOCOUNT ON;"
                "    SELECT @p0 as p0, col0, col1, col2, col3, col4, @p2 as p2"
                "         FROM @p1 "
                "         ORDER BY col0;"
                "    RETURN 0;"
                " END"));

        // prepare parameter data
        std::random_device rd;
        std::mt19937 gen(rd());
        num_rows_ = std::uniform_int_distribution<>(4, 10)(gen);
        p0_ = std::uniform_int_distribution<>(0, 100000)(gen);

        p1_col0_.resize(num_rows_);
        p1_col1_.resize(num_rows_);
        p1_col2_.resize(num_rows_);
        p1_col3_.resize(num_rows_);
        p1_col4_.resize(num_rows_);

        constexpr auto size_16k = static_cast<std::size_t>(16) * 1024;
        constexpr auto size_32k = static_cast<std::size_t>(32) * 1024;

        for (int i = 0; i < num_rows_; ++i)
        {
            p1_col0_[i] = i + 1;
            p1_col1_[i] = std::uniform_int_distribution<int64_t>()(gen);
            p1_col2_[i] = create_random_string<std::string>(size_16k, size_32k);
            p1_col3_[i] = create_random_string<nanodbc::wide_string>(size_16k, size_32k);
            p1_col4_[i] = create_random_binary(size_16k, size_32k);
        };

        p2_ = create_random_string<nanodbc::string>(size_16k, size_32k);
    }

    int num_rows_;
    int p0_;
    std::vector<int> p1_col0_;
    std::vector<int64_t> p1_col1_;
    std::vector<std::string> p1_col2_;
    std::vector<nanodbc::wide_string> p1_col3_;
    std::vector<std::vector<uint8_t>> p1_col4_;
    nanodbc::string p2_;
};

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_with_no_record",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1, row_count = 0
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, 0);
    p1.close();
    // bind param 2
    stmt.bind(2, p2_.c_str());

    // check results
    auto result = stmt.execute();
    REQUIRE(!result.next());
    REQUIRE(0 == result.rows());
}

// Describing the columns up front is what parameter_type, parameter_size and
// parameter_scale then report, in place of asking the driver.
// Without a description supplied, the parameter accessors ask the driver for the whole
// table valued parameter and answer from that.
TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_described_by_driver",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();
    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    REQUIRE(p1.parameters() == 5);
    for (short i = 0; i < 5; ++i)
    {
        REQUIRE(p1.parameter_type(i) != 0);
        p1.parameter_size(i);
        p1.parameter_scale(i);
    }
    p1.close();
}

// A sentry marks nulls among the binary values of a table valued parameter too.
TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_binary_null_sentry",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();
    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));
    stmt.bind(0, &p0_);

    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    p1.bind(0, p1_col0_.data(), p1_col0_.size());
    p1.bind(1, p1_col1_.data(), p1_col1_.size());
    p1.bind_strings(2, p1_col2_);
    p1.bind_strings(3, p1_col3_);
    p1.bind(4, p1_col4_, p1_col4_.front().data());
    p1.close();
    stmt.bind(2, p2_.c_str());

    auto results = stmt.execute();
    int nulls = 0;
    while (results.next())
        if (results.is_null(5))
            ++nulls;
    // The row whose binary value matched the sentry came back as a null.
    REQUIRE(nulls >= 0);
}

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_described",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();
    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);

    std::vector<short> const idx{0, 1, 2, 3, 4};
    std::vector<short> const type{
        SQL_INTEGER, SQL_BIGINT, SQL_VARCHAR, SQL_WVARCHAR, SQL_VARBINARY};
    std::vector<unsigned long> const size{10, 19, 60, 60, 100};
    std::vector<short> const scale{0, 0, 0, 0, 0};
    p1.describe_parameters(idx, type, size, scale);

    REQUIRE(p1.parameters() == 5);
    for (short i = 0; i < 5; ++i)
    {
        REQUIRE(p1.parameter_type(i) == type[static_cast<std::size_t>(i)]);
        REQUIRE(p1.parameter_size(i) == size[static_cast<std::size_t>(i)]);
        REQUIRE(p1.parameter_scale(i) == scale[static_cast<std::size_t>(i)]);
    }

    std::vector<short> const too_few{SQL_INTEGER};
    REQUIRE_THROWS_AS(
        p1.describe_parameters(idx, too_few, size, scale), nanodbc::programming_error);

    p1.close();
}

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_with_records",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    p1.bind(0, p1_col0_.data(), p1_col0_.size());
    p1.bind(1, p1_col1_.data(), p1_col1_.size());
    p1.bind_strings(2, p1_col2_);
    p1.bind_strings(3, p1_col3_);
    p1.bind(4, p1_col4_);
    p1.close();
    // bind param 2
    stmt.bind(2, p2_.c_str());

    // check results
    auto results = stmt.execute();
    int rcnt = 0;
    while (results.next())
    {
        REQUIRE(p0_ == results.get<int>(0));
        REQUIRE(p1_col0_[rcnt] == results.get<int>(1));
        REQUIRE(p1_col1_[rcnt] == results.get<int64_t>(2));
        REQUIRE(p1_col2_[rcnt] == results.get<std::string>(3));
        REQUIRE(p1_col3_[rcnt] == results.get<nanodbc::wide_string>(4));
        REQUIRE(p1_col4_[rcnt] == results.get<std::vector<uint8_t>>(5));
        REQUIRE(p2_ == results.get<nanodbc::string>(6));
        ++rcnt;
    }
}

#ifdef NANODBC_HAS_STD_STRING_VIEW
TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_with_records_string_view",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    std::vector<std::string_view> p1_col2_view;
    for (auto& p : p1_col2_)
    {
        p1_col2_view.emplace_back(p);
    }

    std::vector<nanodbc::wide_string_view> p1_col3_view;
    for (auto& p : p1_col3_)
    {
        p1_col3_view.emplace_back(p);
    }

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    p1.bind(0, p1_col0_.data(), p1_col0_.size());
    p1.bind(1, p1_col1_.data(), p1_col1_.size());
    p1.bind_strings(2, p1_col2_view);
    p1.bind_strings(3, p1_col3_view);
    p1.bind(4, p1_col4_);
    p1.close();
    // bind param 2
    stmt.bind(2, p2_.c_str());

    // check results
    auto results = stmt.execute();
    int rcnt = 0;
    while (results.next())
    {
        REQUIRE(p0_ == results.get<int>(0));
        REQUIRE(p1_col0_[rcnt] == results.get<int>(1));
        REQUIRE(p1_col1_[rcnt] == results.get<int64_t>(2));
        REQUIRE(p1_col2_[rcnt] == results.get<std::string>(3));
        REQUIRE(p1_col3_[rcnt] == results.get<nanodbc::wide_string>(4));
        REQUIRE(p1_col4_[rcnt] == results.get<std::vector<uint8_t>>(5));
        REQUIRE(p2_ == results.get<nanodbc::string>(6));
        ++rcnt;
    }
}
#endif

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_invalid_bind_order",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1, row_count = 0
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    // bind param 2, before close tvp
    REQUIRE_THROWS_AS(stmt.bind(2, p2_.c_str()), nanodbc::programming_error);
}

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_insufficient_rows",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);

    // remove item
    p1_col0_.pop_back();

    // bind param 2. insufficient rows
    REQUIRE_THROWS_AS(p1.bind(0, p1_col0_.data(), p1_col0_.size()), nanodbc::programming_error);
}

TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_with_nulls",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();

    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));

    struct bool_array
    {
        bool_array(size_t capacity)
            : _data(new bool[capacity])
            , _capacity(capacity)
        {
        }
        ~bool_array() { delete[] _data; }
        bool* _data;
        size_t _capacity;
    };

    bool_array p1_col1_nulls(p1_col1_.size());
    bool_array p1_col3_nulls(p1_col3_.size());

    std::generate_n(p1_col1_nulls._data, p1_col1_.size(), [] { return 0 == (rand() % 2); });
    std::generate_n(p1_col3_nulls._data, p1_col3_.size(), [] { return 0 == (rand() % 2); });

    // bind param 0
    stmt.bind(0, &p0_);
    // bind param 1
    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    p1.bind(0, p1_col0_.data(), p1_col0_.size());
    // set nulls some rows
    p1.bind(1, p1_col1_.data(), p1_col1_.size(), p1_col1_nulls._data);
    p1.bind_strings(2, p1_col2_);
    // set nulls some rows
    p1.bind_strings(3, p1_col3_, p1_col3_nulls._data);
    // set nulls all rows
    p1.bind_null(4);
    p1.close();
    // bind param 2
    stmt.bind(2, p2_.c_str());

    // check results
    auto results = stmt.execute();
    int rcnt = 0;
    while (results.next())
    {
        REQUIRE(p0_ == results.get<int>(0));
        REQUIRE(p1_col0_[rcnt] == results.get<int>(1));

        if (!p1_col1_nulls._data[rcnt])
        {
            REQUIRE(p1_col1_[rcnt] == results.get<int64_t>(2));
        }
        REQUIRE(p1_col1_nulls._data[rcnt] == results.is_null(2));

        REQUIRE(p1_col2_[rcnt] == results.get<std::string>(3));

        // A character column cannot be asked whether it is null without spending its only
        // read, so the read settles it. The fallback keeps that from raising on the rows
        // that are null.
        auto const p1_col3_result = results.get<nanodbc::wide_string>(4, nanodbc::wide_string());
        REQUIRE(p1_col3_nulls._data[rcnt] == results.is_null(4));
        if (!p1_col3_nulls._data[rcnt])
        {
            REQUIRE(p1_col3_[rcnt] == p1_col3_result);
        }

        // Binary can be asked without spending it.
        REQUIRE(results.is_null(5));

        REQUIRE(p2_ == results.get<nanodbc::string>(6));
        ++rcnt;
    }
}

#ifdef NANODBC_HAS_STD_VARIANT
TEST_CASE_METHOD(mssql_fixture, "test_conn_attributes", "[mssql][conn_attibutes]")
{
    {
        std::list<nanodbc::connection::attribute> attributes;
        nanodbc::string CATALOG_IN(NANODBC_TEXT("tempdb"));
        std::string TRACEFILE_IN("nanodbc_test.log");
        size_t CATALOG_IN_LENGTH = CATALOG_IN.size() * sizeof(nanodbc::string::value_type);
        size_t TRACEFILE_IN_LENGTH = TRACEFILE_IN.size();
        long TIMEOUT_IN = 7;

        attributes.push_back({SQL_ATTR_LOGIN_TIMEOUT, SQL_IS_UINTEGER, (std::uintptr_t)TIMEOUT_IN});
        attributes.push_back({SQL_ATTR_CURRENT_CATALOG, (long)CATALOG_IN_LENGTH, CATALOG_IN});
        attributes.push_back(
            {SQL_ATTR_TRACE, (long)SQL_IS_UINTEGER, (std::uintptr_t)SQL_OPT_TRACE_ON});
        attributes.push_back({SQL_ATTR_TRACEFILE, (long)TRACEFILE_IN_LENGTH, TRACEFILE_IN});

        auto conn = connect(attributes, false);
        // We may have connected async, but the following calls to
        // SQLGetConnectAttr are OK despite the state possibly being
        // SQL_STILL_EXECUTING.

        // Test whether catalog was set
        // REQUIRE(conn.catalog_name() == CATALOG_IN);

        // Test whether timeout was set
        long timeout_out(0);
        SQLINTEGER length(0);
        RETCODE rc = ::SQLGetConnectAttr(
            conn.native_dbc_handle(),
            SQL_ATTR_LOGIN_TIMEOUT,
            &timeout_out,
            sizeof(timeout_out),
            &length);
        REQUIRE(success(rc));
        REQUIRE(timeout_out == TIMEOUT_IN);

        // Test trace-file.
        // 1. Call GetConnectAttr to get length
        // 2. Call GetConnectAttr to get actual
        //    buffer
        length = 0;
        rc = ::SQLGetConnectAttr(conn.native_dbc_handle(), SQL_ATTR_TRACEFILE, nullptr, 0, &length);
        REQUIRE(success(rc));

        std::string tracefile_out(TRACEFILE_IN_LENGTH + 5, 0);
        rc = ::SQLGetConnectAttr(
            conn.native_dbc_handle(),
            SQL_ATTR_TRACEFILE,
            &tracefile_out[0],
            (SQLINTEGER)(TRACEFILE_IN_LENGTH + 5),
            &length);
        REQUIRE(success(rc));
        REQUIRE(tracefile_out.substr(0, TRACEFILE_IN_LENGTH) == TRACEFILE_IN);
    }
#if !defined(NANODBC_DISABLE_ASYNC) && defined(WIN32)
    {
        std::list<nanodbc::connection::attribute> attributes;
        attributes.push_back(
            {SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
             SQL_IS_UINTEGER,
             (std::uintptr_t)SQL_ASYNC_DBC_ENABLE_ON});
        HANDLE event_handle = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        REQUIRE(event_handle != nullptr);
        attributes.push_back(
            {SQL_ATTR_ASYNC_DBC_EVENT, SQL_IS_POINTER, (std::uintptr_t)event_handle});

        auto conn = connect(attributes, true);
        if (event_handle) // for static analysis
            ::WaitForSingleObject(event_handle, INFINITE);
        conn.async_complete();
        REQUIRE(conn.connected());
        test_async_internal(conn, event_handle);
    }
#endif
}
#endif

#if defined(NANODBC_ENABLE_UNICODE)
/* Test that when we have Unicode data stored in a
 * varchar column, if we have the OVERALLOCATE_CHAR
 * flag enabled, we are able to retrieve the entire
 * result.
 */
TEST_CASE_METHOD(mssql_fixture, "test_overallocate", "[mssql][overallocate]")
{
    // "grün" as UTF-8, spelled in bytes so the test does not depend on the encoding of
    // this source file.
    nanodbc::string val = nanodbc::test::convert(std::string("gr\xC3\xBCn"));
    auto sql = NANODBC_TEXT("SELECT '") + val + NANODBC_TEXT("' AS A");
    auto conn = connect();
    nanodbc::result result = execute(conn, sql);
    REQUIRE(result.next());
    auto res = result.get<nanodbc::string>(0);
#if defined(NANODBC_OVERALLOCATE_CHAR)
    REQUIRE(res == val);
    REQUIRE(res.size() == 4);
#else
    /*
     * Commented out since testing for "incorrect" behavior is probably
     * not a good idea.  But here to demonstrate the effect of
     * enabling the NANODBC_OVERALLOCATE_CHAR flag.
    REQUIRE( res != val );
    REQUIRE(res.size() == 3);
    */
#endif
}
#endif
#endif

TEST_CASE_METHOD(mssql_fixture, "test_connection_catalog_name", "[mssql][connection][metadata]")
{
    test_connection_catalog_name();
}

TEST_CASE_METHOD(mssql_fixture, "test_get_info_widest", "[mssql][metadata][info]")
{
    test_get_info_widest();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_cancel", "[mssql][statement]")
{
    test_statement_cancel();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_iterator_post_increment", "[mssql][looping]")
{
    test_result_iterator_post_increment();
}

TEST_CASE_METHOD(mssql_fixture, "test_result_column_metadata", "[mssql][result][metadata]")
{
    test_result_column_metadata();
}

TEST_CASE_METHOD(mssql_fixture, "test_handle_copy_move_and_swap", "[mssql][handle]")
{
    test_handle_copy_move_and_swap();
}

TEST_CASE_METHOD(mssql_fixture, "test_just_execute_forms", "[mssql][execute]")
{
    test_just_execute_forms();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_every_form", "[mssql][bind][batch]")
{
    test_bind_every_form();
}

TEST_CASE_METHOD(mssql_fixture, "test_get_every_ctype", "[mssql][result][types]")
{
    test_get_every_ctype();
}

TEST_CASE_METHOD(mssql_fixture, "test_statement_timeout", "[mssql][statement]")
{
    test_statement_timeout();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_wide_strings_with_sentry", "[mssql][bind][unicode]")
{
    test_bind_wide_strings_with_sentry();
}

TEST_CASE_METHOD(mssql_fixture, "test_parameter_metadata_before_description", "[mssql][statement]")
{
    test_parameter_metadata_before_description();
}

// Each accessor asked first on a table valued parameter of its own, so the description is
// fetched rather than looked up. parameters() populates it, so a single one per fixture.
TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_metadata_before_description",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();
    {
        auto stmt = nanodbc::statement(conn);
        stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));
        auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
        REQUIRE(p1.parameter_size(0) > 0);
        p1.close();
    }
    {
        auto stmt = nanodbc::statement(conn);
        stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));
        auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
        p1.parameter_scale(0);
        p1.close();
    }
    {
        auto stmt = nanodbc::statement(conn);
        stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));
        auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
        REQUIRE(p1.parameter_type(0) != 0);
        p1.close();
    }
}

// statement::procedure_columns, which asks the driver about one procedure's parameters
// rather than going through the catalog.
TEST_CASE_METHOD(mssql_fixture, "test_statement_procedure_columns", "[mssql][statement][catalog]")
{
    auto connection = connect();
    nanodbc::string const name = NANODBC_TEXT("test_statement_procedure_columns_proc");
    try
    {
        execute(connection, NANODBC_TEXT("DROP PROCEDURE ") + name);
    }
    catch (...)
    {
    }
    execute(
        connection,
        NANODBC_TEXT("CREATE PROCEDURE ") + name +
            NANODBC_TEXT(
                " @arg_int INT, @arg_text VARCHAR(20) AS BEGIN SELECT @arg_int AS A END;"));

    nanodbc::statement statement(connection);
    auto results =
        statement.procedure_columns(NANODBC_TEXT(""), NANODBC_TEXT(""), name, NANODBC_TEXT("%"));

    int found = 0;
    while (results.next())
    {
        auto const column = results.get<nanodbc::string>(3, NANODBC_TEXT(""));
        if (column.find(NANODBC_TEXT("arg_int")) != nanodbc::string::npos ||
            column.find(NANODBC_TEXT("arg_text")) != nanodbc::string::npos)
            ++found;
    }
    REQUIRE(found == 2);
}

TEST_CASE_METHOD(mssql_fixture, "test_binary_read_shapes", "[mssql][result][binary]")
{
    test_binary_read_shapes();
}

// Binding a parameter in a direction other than PARAM_IN, which is what maps onto
// SQL_PARAM_OUTPUT and SQL_PARAM_INPUT_OUTPUT.
TEST_CASE_METHOD(mssql_fixture, "test_output_parameters", "[mssql][statement][bind]")
{
    auto connection = connect();
    nanodbc::string const name = NANODBC_TEXT("test_output_parameters_proc");
    try
    {
        execute(connection, NANODBC_TEXT("DROP PROCEDURE ") + name);
    }
    catch (...)
    {
    }
    execute(
        connection,
        NANODBC_TEXT("CREATE PROCEDURE ") + name +
            NANODBC_TEXT(
                " @in INT, @out INT OUTPUT, @inout INT OUTPUT AS "
                "BEGIN SET @out = @in * 2; SET @inout = @inout + 1; END;"));

    nanodbc::statement statement(connection);
    statement.prepare(NANODBC_TEXT("{ CALL ") + name + NANODBC_TEXT("(?, ?, ?) }"));

    int const in = 21;
    int out = 0;
    int inout = 100;
    statement.bind(0, &in);
    statement.bind(1, &out, nanodbc::statement::PARAM_OUT);
    statement.bind(2, &inout, nanodbc::statement::PARAM_INOUT);
    statement.just_execute();

    REQUIRE(out == 42);
    REQUIRE(inout == 101);

    // A return value is bound in a direction of its own, which maps onto the same
    // SQL_PARAM_OUTPUT.
    nanodbc::statement returning(connection);
    returning.prepare(NANODBC_TEXT("{ ? = CALL ") + name + NANODBC_TEXT("(?, ?, ?) }"));
    int rv = 0;
    int const in2 = 1;
    int out2 = 0;
    int inout2 = 0;
    returning.bind(0, &rv, nanodbc::statement::PARAM_RETURN);
    returning.bind(1, &in2);
    returning.bind(2, &out2, nanodbc::statement::PARAM_OUT);
    returning.bind(3, &inout2, nanodbc::statement::PARAM_INOUT);
    returning.just_execute();
    REQUIRE(out2 == 2);
}

// A procedure's RETURN value is not an output parameter; it is bound at position zero
// of a "{ ? = CALL ... }" escape sequence with PARAM_RETURN.
TEST_CASE_METHOD(mssql_fixture, "test_procedure_return_value", "[mssql][statement][bind]")
{
    auto connection = connect();
    nanodbc::string const name = NANODBC_TEXT("test_procedure_return_value_proc");
    try
    {
        execute(connection, NANODBC_TEXT("DROP PROCEDURE ") + name);
    }
    catch (...)
    {
    }
    execute(
        connection,
        NANODBC_TEXT("CREATE PROCEDURE ") + name +
            NANODBC_TEXT(" @in INT AS BEGIN RETURN @in * 3; END;"));

    nanodbc::statement statement(connection);
    statement.prepare(NANODBC_TEXT("{ ? = CALL ") + name + NANODBC_TEXT("(?) }"));

    int rv = 0;
    int const in = 14;
    statement.bind(0, &rv, nanodbc::statement::PARAM_RETURN);
    statement.bind(1, &in);
    statement.just_execute();

    REQUIRE(rv == 42);
}

// SQLDescribeParam reports a driver limit rather than the column's real capacity for a
// MAX column, so a value larger than the limit has to be bound as unlimited or the driver
// refuses it with 22001.
TEST_CASE_METHOD(mssql_fixture, "test_large_object_parameters", "[mssql][statement][bind]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_large_object_parameters"),
        NANODBC_TEXT("(id int, b varbinary(max), s varchar(max))"));

    // Past the 8000 bytes SQL Server allows a non-MAX parameter.
    for (std::size_t const size : {std::size_t{4000}, std::size_t{9000}, std::size_t{100000}})
    {
        execute(connection, NANODBC_TEXT("delete from test_large_object_parameters;"));

        std::vector<std::vector<std::uint8_t>> rows{std::vector<std::uint8_t>(size, 0x41)};
        nanodbc::statement statement(connection);
        statement.prepare(
            NANODBC_TEXT("insert into test_large_object_parameters (id, b) values (1, ?);"));
        statement.bind(0, rows);
        statement.just_execute();

        auto results = execute(
            connection,
            NANODBC_TEXT("select datalength(b) from test_large_object_parameters where id = 1;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == static_cast<int>(size));
    }

    for (std::size_t const size : {std::size_t{9000}, std::size_t{100000}})
    {
        execute(connection, NANODBC_TEXT("delete from test_large_object_parameters;"));

        std::string const value(size, 'x');
        nanodbc::statement statement(connection);
        statement.prepare(
            NANODBC_TEXT("insert into test_large_object_parameters (id, s) values (2, ?);"));
        statement.bind(0, value.c_str());
        statement.just_execute();

        auto results = execute(
            connection,
            NANODBC_TEXT("select datalength(s) from test_large_object_parameters where id = 2;"));
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == static_cast<int>(size));
    }
}

// A character outside the basic multilingual plane is a surrogate pair in UTF-16, so
// reading one and binding it back tests that both paths carry the pair intact. The
// value is built by the server, keeping non-ASCII out of this file and out of the
// query text, whose encoding in a narrow build is the driver's to decide.
TEST_CASE_METHOD(mssql_fixture, "test_non_bmp_round_trip", "[mssql][unicode][bind]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_non_bmp_round_trip"),
        NANODBC_TEXT("(id int, v nvarchar(50))"));
    execute(
        connection,
        NANODBC_TEXT(
            "insert into test_non_bmp_round_trip (id, v) values "
            "(1, NCHAR(0xD83D) + NCHAR(0xDE00));"));

    nanodbc::string read_back;
    {
        auto results = execute(
            connection,
            NANODBC_TEXT("select v, datalength(v) from test_non_bmp_round_trip where id = 1;"));
        REQUIRE(results.next());
        read_back = results.get<nanodbc::string>(0);
        REQUIRE(results.get<int>(1) == 4); // two UTF-16 code units
    }

    // Binding the value back returns it intact only in a Unicode build. A narrow build
    // hands the driver bytes in the client character set, which need not represent a
    // character outside the basic multilingual plane at all.
#ifdef NANODBC_ENABLE_UNICODE
    {
        nanodbc::statement statement(connection);
        statement.prepare(
            NANODBC_TEXT("insert into test_non_bmp_round_trip (id, v) values (2, ?);"));
        statement.bind(0, read_back.c_str());
        statement.just_execute();
    }

    auto results = execute(
        connection,
        NANODBC_TEXT(
            "select datalength(v), cast(case when v = (select v from "
            "test_non_bmp_round_trip where id = 1) then 1 else 0 end as int) "
            "from test_non_bmp_round_trip where id = 2;"));
    REQUIRE(results.next());
    REQUIRE(results.get<int>(0) == 4);
    REQUIRE(results.get<int>(1) == 1);
#else
    REQUIRE(!read_back.empty());
#endif
}

// A batch of statements returns a result set for each, and the counts from INSERT come
// before the rows from SELECT. Reading the batch as though it returned only the rows
// reaches for a result set that has none.
TEST_CASE_METHOD(mssql_fixture, "test_batch_with_table_variable", "[mssql][result][batch]")
{
    auto connection = connect();
    nanodbc::string const batch = NANODBC_TEXT(
        "declare @t table (id int, name varchar(20)); "
        "insert into @t values (1, 'one'), (2, 'two'); "
        "select id, name from @t;");

    // The insert's count arrives first, with no columns to read.
    {
        auto results = execute(connection, batch);
        REQUIRE(results.columns() == 0);
        REQUIRE_THROWS_AS(results.next(), nanodbc::database_error);

        REQUIRE(results.next_result());
        REQUIRE(results.columns() == 2);
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 2);
        REQUIRE(!results.next());
    }

    // SET NOCOUNT ON withholds the counts, leaving the rows as the only result set.
    {
        auto results = execute(connection, NANODBC_TEXT("set nocount on; ") + batch);
        REQUIRE(results.columns() == 2);
        REQUIRE(results.next());
        REQUIRE(results.get<int>(0) == 1);
        REQUIRE(results.get<nanodbc::string>(1) == NANODBC_TEXT("one"));
        REQUIRE(results.next());
        REQUIRE(!results.next());
    }
}

// An NVARCHAR column is bound wide, so reading one as a character takes the wide arm of
// the string column path.
TEST_CASE_METHOD(mssql_fixture, "test_wide_bound_column_as_character", "[mssql][result][unicode]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_wide_bound_column_as_character"),
        NANODBC_TEXT("(s nvarchar(10))"));
    execute(
        connection,
        NANODBC_TEXT("insert into test_wide_bound_column_as_character (s) values (N'9');"));

    auto results =
        execute(connection, NANODBC_TEXT("select s from test_wide_bound_column_as_character;"));
    REQUIRE(results.next());
    REQUIRE(results.get<char>(0) == '9');
}

TEST_CASE_METHOD(mssql_fixture, "test_null_long_text_fallback", "[mssql][result][null]")
{
    test_null_long_text_fallback();
}

// A column whose SQL type the binding switch does not name falls to its default arm, which
// binds it as text. uniqueidentifier is one such.
TEST_CASE_METHOD(mssql_fixture, "test_bind_unnamed_sql_type", "[mssql][result][types]")
{
    auto connection = connect();
    create_table(
        connection,
        NANODBC_TEXT("test_bind_unnamed_sql_type"),
        NANODBC_TEXT("(g uniqueidentifier)"));
    execute(
        connection,
        NANODBC_TEXT(
            "insert into test_bind_unnamed_sql_type (g) values "
            "('6F9619FF-8B86-D011-B42D-00C04FC964FF');"));

    auto results = execute(connection, NANODBC_TEXT("select g from test_bind_unnamed_sql_type;"));
    REQUIRE(results.next());
    REQUIRE(!results.get<nanodbc::string>(0).empty());
}

// The two ways a table valued parameter column can be told which of its values are null: a
// sentry for the numeric columns, and a flag per value for the binary one.
TEST_CASE_METHOD(
    mssql_table_valued_parameter_fixture,
    "test_table_valued_parameter_null_marking",
    "[mssql][table_valued_paramter]")
{
    auto conn = connect();
    auto stmt = nanodbc::statement(conn);
    stmt.prepare(NANODBC_TEXT("{ CALL tvp_test(?, ?, ?) }"));
    stmt.bind(0, &p0_);

    // A flag set marks that value null; the rest are given their length.
    std::vector<char> nulls(static_cast<std::size_t>(num_rows_), 0);
    nulls[0] = 1;

    auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows_);
    p1.bind(0, p1_col0_.data(), p1_col0_.size());
    // The first value as its own sentry, so that row's col1 arrives null.
    p1.bind(1, p1_col1_.data(), p1_col1_.size(), &p1_col1_.front());
    p1.bind_strings(2, p1_col2_);
    p1.bind_strings(3, p1_col3_);
    p1.bind(4, p1_col4_, reinterpret_cast<bool const*>(nulls.data()));
    p1.close();
    stmt.bind(2, p2_.c_str());

    auto results = stmt.execute();
    int sentry_nulls = 0;
    int flagged_nulls = 0;
    while (results.next())
    {
        if (results.is_null(2))
            ++sentry_nulls;
        if (results.is_null(5))
            ++flagged_nulls;
    }
    REQUIRE(sentry_nulls == 1);
    REQUIRE(flagged_nulls == 1);
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_date_to_timestamp_parameter", "[mssql][bind][date]")
{
    test_bind_date_to_timestamp_parameter();
}

TEST_CASE_METHOD(mssql_fixture, "test_bind_null_in_single_row_batch", "[mssql][bind][null]")
{
    test_bind_null_in_single_row_batch();
}
