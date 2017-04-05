#!/bin/bash -ue

sudo odbcinst -i -d -f /usr/share/psqlodbc/odbcinst.ini.template
psql -c "CREATE DATABASE nanodbc_tests;" -U postgres
export NANODBC_TEST_CONNSTR_PGSQL="DRIVER={PostgreSQL ANSI};Server=localhost;Port=5432;Database=nanodbc_tests;UID=postgres;"
