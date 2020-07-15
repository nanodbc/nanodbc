#!/bin/bash -ue

sudo odbcinst -i -d -f /usr/share/psqlodbc/odbcinst.ini.template
psql -c "CREATE DATABASE nanodbc_tests;" -U postgres
if [[ "$ENABLE_UNICODE" == "ON" ]]; then
    ODBC_DRIVER_NAME="PostgreSQL Unicode"
else
    ODBC_DRIVER_NAME="PostgreSQL ANSI"
fi
export NANODBC_TEST_CONNSTR_PGSQL="DRIVER={${ODBC_DRIVER_NAME}};Server=localhost;Port=5432;Database=nanodbc_tests;UID=postgres;"
cat > ${HOME}/.odbc.ini <<EOF
[testdsn]
Driver                  = ${ODBC_DRIVER_NAME}
EOF
