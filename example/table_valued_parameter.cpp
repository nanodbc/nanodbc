#include "example_unicode_utils.h"
#include <nanodbc/nanodbc.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using namespace std;
using namespace nanodbc;

// returns random string [min_size, max_size]
template <class T, typename = nanodbc::enable_if_string<T>>
T create_random_string(size_t min_size, size_t max_size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist_size(min_size, max_size);
    std::uniform_int_distribution<size_t> dist_alpha(0, 25);

    T result;
    result.resize(dist_size(gen));

    for (auto& dst : result)
    {
        // set 'A' to 'Z'
        dst = static_cast<typename T::value_type>('A') +
              static_cast<typename T::value_type>(dist_alpha(gen));
    }

    return result;
}

// returns random binary [min_size, max_size]
std::vector<uint8_t> create_random_binary(size_t min_size, size_t max_size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist_size(min_size, max_size);
    std::uniform_int_distribution<size_t> dist_bin(0, 255);

    std::vector<uint8_t> result;
    result.resize(dist_size(gen));

    for (auto& dst : result)
    {
        // set 0x00 to 0xFF
        dst = static_cast<uint8_t>(dist_bin(gen));
    }

    return result;
}

void execute_silent(nanodbc::connection& connection, nanodbc::string const& query)
{
    try
    {
        execute(connection, query);
    }
    catch (...)
    {
    }
}

void run_test(nanodbc::string const& connection_string)
{
#ifndef NANODBC_DISABLE_MSSQL_TVP
    nanodbc::connection connection(connection_string);

    // drop stored procedure
    execute_silent(connection, NANODBC_TEXT("DROP PROCEDURE dbo.TVPTest;"));
    // drop tvp_param
    execute_silent(connection, NANODBC_TEXT("DROP TYPE dbo.TVPParam;"));

    // create tvp_param
    execute(
        connection,
        NANODBC_TEXT("CREATE TYPE dbo.TVPParam AS TABLE "
                     " (col0 INT,"
                     "  col1 VARCHAR(MAX),"
                     "  col2 NVARCHAR(MAX),"
                     "  col3 VARBINARY(MAX));"));
    // create stored procedure
    execute(
        connection,
        NANODBC_TEXT(
            "CREATE PROCEDURE dbo.TVPTest(@p0 INT, @p1 dbo.TVPParam READONLY, @p2 NVARCHAR(max)) "
            "AS "
            "BEGIN "
            "SET NOCOUNT ON; "
            "SELECT @p0 as p0, col0, col1, col2, col3, @p2 as p2 FROM @p1 ORDER BY col0; "
            "RETURN 0; "
            "END"));

    // init parameter values...
    std::random_device rd;
    std::mt19937 gen(rd());
    int num_rows = std::uniform_int_distribution<>(4, 10)(gen);
    int p0 = std::uniform_int_distribution<>(0, 100000)(gen);
    auto p2 = create_random_string<nanodbc::string>(16 * 1024, 32 * 1024);

    vector<int> p1_col0;
    vector<std::string> p1_col1;
    vector<nanodbc::wide_string> p1_col2;
    vector<vector<uint8_t>> p1_col3;

    p1_col0.resize(num_rows);
    p1_col1.resize(num_rows);
    p1_col2.resize(num_rows);
    p1_col3.resize(num_rows);

    for (int i = 0; i < num_rows; ++i)
    {
        p1_col0[i] = i + 1;
        p1_col1[i] = create_random_string<std::string>(16 * 1024, 32 * 1024);
        p1_col2[i] = create_random_string<nanodbc::wide_string>(16 * 1024, 32 * 1024);
        p1_col3[i] = create_random_binary(16 * 1024, 32 * 1024);
    };

    {
        auto stmt = nanodbc::statement(connection);
        stmt.prepare(NANODBC_TEXT("{ CALL dbo.TVPTest(?, ?, ?) }"));

        // bind param 0
        stmt.bind(0, &p0);
        // bind param 1
        auto p1 = nanodbc::table_valued_parameter(stmt, 1, num_rows);
        p1.bind(0, p1_col0.data(), p1_col0.size());
        p1.bind_strings(1, p1_col1);
        p1.bind_strings(2, p1_col2);
        p1.bind(3, p1_col3);
        p1.close();
        // bind param 2
        stmt.bind(2, p2.c_str());

        // check results
        auto results = stmt.execute();
        int rcnt = 0;
        while (results.next())
        {
            if (p0 != results.get<int>(NANODBC_TEXT("p0")))
                throw runtime_error("invalid @p0");
            if (p1_col0[rcnt] != results.get<int>(NANODBC_TEXT("col0")))
                throw runtime_error("invalid @p1 col0");
            if (p1_col1[rcnt] != results.get<std::string>(NANODBC_TEXT("col1")))
                throw runtime_error("invalid @p1 col1");
            if (p1_col2[rcnt] != results.get<nanodbc::wide_string>(NANODBC_TEXT("col2")))
                throw runtime_error("invalid @p1 col2");
            if (p1_col3[rcnt] != results.get<vector<uint8_t>>(NANODBC_TEXT("col3")))
                throw runtime_error("invalid @p1 col3");
            if (p2 != results.get<nanodbc::string>(NANODBC_TEXT("p2")))
                throw runtime_error("invalid @p2");
            ++rcnt;
        }

        cout << "test success, rows:" << rcnt << endl;
    }

    // drop stored procedure
    execute_silent(connection, NANODBC_TEXT("DROP PROCEDURE dbo.TVPTest;"));
    // drop tvp_param
    execute_silent(connection, NANODBC_TEXT("DROP TYPE dbo.TVPParam;"));
#else
    cout << "doesn't support table-valued parameter" << endl;
#endif
}

void usage(ostream& out, std::string const& binary_name)
{
    out << "usage: " << binary_name << " connection_string" << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        char* app_name = strrchr(argv[0], '/');
        app_name = app_name ? app_name + 1 : argv[0];
        if (0 == strncmp(app_name, "lt-", 3))
            app_name += 3; // remove libtool prefix
        usage(cerr, app_name);
        return EXIT_FAILURE;
    }

    try
    {
        auto const connection_string(convert(argv[1]));
        run_test(connection_string);
        return EXIT_SUCCESS;
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
    }
    return EXIT_FAILURE;
}
