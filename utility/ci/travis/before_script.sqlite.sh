#!/bin/bash -ue

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    DRIVER="$(brew ls -v sqliteodbc | grep libsqlite3odbc.dylib)"
    cat >"$(odbc_config --odbcinstini)" <<EOF
[SQLite3]
Description             = SQLite3 ODBC Driver
Setup                   = $DRIVER
Driver                  = $DRIVER
Threading               = 2

EOF
else
    sudo odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini
fi
ODBC_DRIVER_NAME="SQLite3"
cat > ${HOME}/.odbc.ini <<EOF
[testdsn]
Driver                  = ${ODBC_DRIVER_NAME}
EOF
