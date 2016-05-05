#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
    struct mssql_fixture : public base_test_fixture
    {
        mssql_fixture()
        : base_test_fixture(/* connecting string from NANODBC_TEST_CONNSTR environment variable)*/)
        {
        }

        virtual ~mssql_fixture() NANODBC_NOEXCEPT
        {
        }
    };
}

// TODO: catlog_* tests

TEST_CASE_METHOD(mssql_fixture, "dbms_info_test", "[mssql][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(mssql_fixture, "decimal_conversion_test", "[mssql][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(mssql_fixture, "exception_test", "[mssql][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(mssql_fixture, "execute_multiple_transaction_test", "[mssql][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(mssql_fixture, "execute_multiple_test", "[mssql][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(mssql_fixture, "integral_test", "[mssql][integral]")
{
    integral_test<mssql_fixture>();
}

TEST_CASE_METHOD(mssql_fixture, "move_test", "[mssql][move]")
{
    move_test();
}

TEST_CASE_METHOD(mssql_fixture, "null_test", "[mssql][null]")
{
    null_test();
}

TEST_CASE_METHOD(mssql_fixture, "nullptr_nulls_test", "[mssql][null]")
{
    nullptr_nulls_test();
}

TEST_CASE_METHOD(mssql_fixture, "simple_test", "[mssql]")
{
    simple_test();
}

TEST_CASE_METHOD(mssql_fixture, "string_test", "[mssql][string]")
{
    string_test();
}

TEST_CASE_METHOD(mssql_fixture, "transaction_test", "[mssql][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(mssql_fixture, "while_not_end_iteration_test", "[mssql][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(mssql_fixture, "while_next_iteration_test", "[mssql][looping]")
{
    while_next_iteration_test();
}
