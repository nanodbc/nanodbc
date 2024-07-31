FROM ubuntu:22.04

ENV CXX="g++-11"
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get -qy update \
    && apt-get -qy install --no-upgrade --no-install-recommends \
    apt-transport-https \
    apt-utils \
    clang-format-15 \
    cmake \
    curl \
    g++ \
    git \
    gpg-agent \
    iputils-ping \
    libsqliteodbc \
    libssl-dev \
    locales \
    make \
    mysql-client \
    odbc-postgresql \
    postgresql-client \
    software-properties-common \
    sqlite3 \
    unixodbc \
    unixodbc-dev \
    vim \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# NOTE: To install ODBC 18 on Ubuntu, see: https://learn.microsoft.com/en-us/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server
#       Keep in mind that only certain releases of Ubuntu are supported.
RUN curl https://packages.microsoft.com/keys/microsoft.asc | tee /etc/apt/trusted.gpg.d/microsoft.asc \
    && curl "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/prod.list" | tee /etc/apt/sources.list.d/mssql-release.list \
    && apt-get -qy update \
    && ACCEPT_EULA=Y apt-get -qy install --no-upgrade --no-install-recommends \
    msodbcsql18 \
    mssql-tools18 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && echo "export PATH=$PATH:/opt/mssql-tools18/bin" >> ~/.bash_profile \
    && echo "export PATH=$PATH:/opt/mssql-tools18/bin" >> ~/.bashrc \
    && echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
    && locale-gen \
    && odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini


# NOTE: To install mysql-connector-odbc on Ubuntu, see: https://dev.mysql.com/downloads/connector/odbc/
#       Keep in mind that only certain architectures are supported.
RUN curl -SL "https://downloads.mysql.com/archives/get/p/10/file/mysql-connector-odbc-8.4.0-linux-glibc2.28-$(uname -m).tar.gz" -o mysql-connector-odbc.tar.gz \
    && tar -xzf mysql-connector-odbc.tar.gz \
    && rm mysql-connector-odbc.tar.gz \
    && mv mysql-connector-odbc* /opt/mysql-connector-odbc \
    && /opt/mysql-connector-odbc/bin/myodbc-installer -d -a -n "MySQL ODBC 8.4 ANSI Driver" -t "DRIVER=/opt/mysql-connector-odbc/lib/libmyodbc8a.so;" \
    && /opt/mysql-connector-odbc/bin/myodbc-installer -d -a -n "MySQL ODBC 8.4 Unicode Driver" -t "DRIVER=/opt/mysql-connector-odbc/lib/libmyodbc8w.so;"

# NOTE: To install latest version of CMake on Ubuntu, see: https://apt.kitware.com/
RUN curl -SL https://github.com/Kitware/CMake/releases/download/v3.30.1/cmake-3.30.1.tar.gz -o cmake.tar.gz \
    && tar -xzf cmake.tar.gz \
    && rm cmake.tar.gz \
    && mv cmake-* /opt/cmake
WORKDIR /opt/cmake
RUN ./bootstrap \
    && make -j"$(nproc)" \
    && make install

WORKDIR /
RUN git clone https://github.com/nanodbc/nanodbc.git /opt/nanodbc \
    && mkdir -p /opt/nanodbc/build
