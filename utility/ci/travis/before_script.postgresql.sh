#!/bin/bash -ue

sudo odbcinst -i -d -f /usr/share/psqlodbc/odbcinst.ini.template
psql -c "CREATE DATABASE nanodbc_tests;" -U postgres
if [[ "$ENABLE_UNICODE" == "ON" ]]; then
    POSTGRESQL_DRIVER="{PostgreSQL Unicode}"
else
    POSTGRESQL_DRIVER="{PostgreSQL ANSI}"
fi
export NANODBC_TEST_CONNSTR_PGSQL="DRIVER=${POSTGRESQL_DRIVER};Server=localhost;Port=5432;Database=nanodbc_tests;UID=postgres;"