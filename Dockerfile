FROM ubuntu:latest
RUN DEBIAN_FRONTEND=noninteractive apt-get -qqy update \
 && DEBIAN_FRONTEND=noninteractive apt-get -qqy install software-properties-common

RUN DEBIAN_FRONTEND=noninteractive add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN DEBIAN_FRONTEND=noninteractive apt-get update -qqy \
 && DEBIAN_FRONTEND=noninteractive apt-get -qqy install \
        $(apt-cache -q search "libboost-locale1\..*-dev" | awk '{print $1}' | sort -nr | head -n 1) \
        cmake \
        doxygen \
        g++-5 \
        git \
        make \
        mysql-client \
        mysql-server \
        sqlite3 \
        unixodbc \
        unixodbc-dev \
        vim

# Might not be available, but install it if it is.
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install jekyll || true

# NOTE: `libmyodbc`, the package for MySQL ODBC support, is no longer available directly via a
# simple `apt-get install libmyodbc` command. Instead, you must install it manually. The following
# blog post provides step-by-step instructions.
# https://www.datasunrise.com/blog/how-to-install-the-mysql-odbc-driver-on-ubuntu-16-04/

RUN odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini

ENV CXX g++-5
