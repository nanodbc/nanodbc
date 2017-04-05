#!/bin/bash -ue

sudo cp "${TRAVIS_BUILD_DIR}/utility/ci/odbcinst.ini.vertica" /etc/odbcinst.ini
sudo cp "${TRAVIS_BUILD_DIR}/utility/ci/vertica.ini.vertica" /etc/vertica.ini
export NANODBC_TEST_CONNSTR_VERTICA="DRIVER={Vertica};ServerName=localhost;Port=5433;Database=ci;UID=dbadmin;PWD=dbadmin;"
export VERTICAINI=/etc/vertica.ini
