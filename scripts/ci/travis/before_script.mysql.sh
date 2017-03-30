#!/bin/bash -ue

sudo odbcinst -i -d -f /usr/share/libmyodbc/odbcinst.ini
mysql -e "DROP DATABASE IF EXISTS nanodbc_tests;" -uroot
mysql -e "CREATE DATABASE IF NOT EXISTS nanodbc_tests;" -uroot
mysql -e "GRANT ALL PRIVILEGES ON *.* TO 'root'@'localhost';" -uroot
export NANODBC_TEST_CONNSTR_MYSQL="Driver=MySQL;Server=localhost;Database=nanodbc_tests;User=root;Password=;Option=3;big_packets=1;"
