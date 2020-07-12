#!/bin/bash -ue

sudo cp "${TRAVIS_BUILD_DIR}/utility/ci/odbcinst.ini.vertica" /etc/odbcinst.ini
sudo cp "${TRAVIS_BUILD_DIR}/utility/ci/vertica.ini.vertica" /etc/vertica.ini
ODBC_DRIVER_NAME="{Vertica}"
export NANODBC_TEST_CONNSTR_VERTICA="Driver=${ODBC_DRIVER_NAME};ServerName=localhost;Port=5433;Database=ci;UID=dbadmin;PWD=dbadmin;"
export VERTICAINI=/etc/vertica.ini
cat > ${HOME}/.odbc.ini <<EOF
[testdsn]
Driver                  = ${ODBC_DRIVER_NAME}
EOF
