#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "test/base_test_fixture.h"
#include <cstdio>
#include <cstdlib>

namespace
{
    struct postgresql_fixture : public base_test_fixture
    {
        postgresql_fixture()
        : base_test_fixture(/* connecting string from NANODBC_TEST_CONNSTR environment variable)*/)
        {
            if (connection_string_.empty())
                connection_string_ = get_env("NANODBC_TEST_CONNSTR_PGSQL");
        }

        virtual ~postgresql_fixture() NANODBC_NOEXCEPT
        {
        }
    };
}

// TODO: add blob (bytea) test

TEST_CASE_METHOD(postgresql_fixture, "catalog_columns_test", "[postgresql][catalog][columns]")
{
    catalog_columns_test();
}

TEST_CASE_METHOD(postgresql_fixture, "catalog_primary_keys_test", "[postgresql][catalog][primary_keys]")
{
    catalog_primary_keys_test();
}

TEST_CASE_METHOD(postgresql_fixture, "catalog_tables_test", "[postgresql][catalog][tables]")
{
    catalog_tables_test();
}

TEST_CASE_METHOD(postgresql_fixture, "dbms_info_test", "[postgresql][dmbs][metadata][info]")
{
    dbms_info_test();
}

TEST_CASE_METHOD(postgresql_fixture, "decimal_conversion_test", "[postgresql][decimal][conversion]")
{
    decimal_conversion_test();
}

TEST_CASE_METHOD(postgresql_fixture, "exception_test", "[postgresql][exception]")
{
    exception_test();
}

TEST_CASE_METHOD(postgresql_fixture, "execute_multiple_transaction_test", "[postgresql][execute][transaction]")
{
    execute_multiple_transaction_test();
}

TEST_CASE_METHOD(postgresql_fixture, "execute_multiple_test", "[postgresql][execute]")
{
    execute_multiple_test();
}

TEST_CASE_METHOD(postgresql_fixture, "integral_test", "[postgresql][integral]")
{
    integral_test<postgresql_fixture>();
}

TEST_CASE_METHOD(postgresql_fixture, "move_test", "[postgresql][move]")
{
    move_test();
}

TEST_CASE_METHOD(postgresql_fixture, "null_test", "[postgresql][null]")
{
    null_test();
}

TEST_CASE_METHOD(postgresql_fixture, "result_iterator_test", "[postgresql][iterator]")
{
    result_iterator_test();
}

TEST_CASE_METHOD(postgresql_fixture, "simple_test", "[postgresql]")
{
    simple_test();
}

TEST_CASE_METHOD(postgresql_fixture, "string_test", "[postgresql][string]")
{
    string_test();
}

TEST_CASE_METHOD(postgresql_fixture, "transaction_test", "[postgresql][transaction]")
{
    transaction_test();
}

TEST_CASE_METHOD(postgresql_fixture, "while_not_end_iteration_test", "[postgresql][looping]")
{
    while_not_end_iteration_test();
}

TEST_CASE_METHOD(postgresql_fixture, "while_next_iteration_test", "[postgresql][looping]")
{
    while_next_iteration_test();
}
