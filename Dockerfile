FROM ubuntu:22.10
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# NOTE: install apt-utils since it is Priority: important, should really be installed otherwise
#       'debconf: delaying package configuration, since apt-utils is not installed'
RUN apt-get -qy update \
 && apt-get -qy install --no-upgrade --no-install-recommends \
        apt-transport-https \
        apt-utils \
        curl \
        software-properties-common \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* \
 && curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add - \
 && apt-get -qy update && apt-get -qy install --no-upgrade --no-install-recommends \
        cmake \
        g++ \
        git \
        iputils-ping \
        make \
        mysql-client \
        libsqliteodbc \
        locales \
        odbc-postgresql \
        postgresql-client \
        sqlite3 \
        unixodbc \
        unixodbc-dev \
        vim \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* \
 && ACCEPT_EULA=Y apt-get -qy install --no-upgrade --no-install-recommends \
        msodbcsql \
        mssql-tools \
 && echo "export PATH=$PATH:/opt/mssql-tools/bin" >> ~/.bash_profile \
 && echo "export PATH=$PATH:/opt/mssql-tools/bin" >> ~/.bashrc \
 && echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
 && locale-gen \
 && odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini

RUN curl -SL https://dev.mysql.com/get/Downloads/Connector-ODBC/5.3/mysql-connector-odbc-5.3.13-linux-ubuntu18.04-x86-64bit.tar.gz | tar -zxC /opt \
 && cp /opt/mysql-connector-odbc-5.3.14-linux-ubuntu17.04-x86-64bit/lib/libmyodbc5* /usr/lib/x86_64-linux-gnu/odbc/ \
 && /opt/mysql-connector-odbc-5.3.13-linux-ubuntu17.04-x86-64bit/bin/myodbc-installer -d -a -n "MySQL ODBC 5.3 ANSI Driver" -t "DRIVER=/usr/lib/x86_64-linux-gnu/odbc/libmyodbc5a.so;" \
 && /opt/mysql-connector-odbc-5.3.13-linux-ubuntu17.04-x86-64bit/bin/myodbc-installer -d -a -n "MySQL ODBC 5.3 Unicode Driver" -t "DRIVER=/usr/lib/x86_64-linux-gnu/odbc/libmyodbc5w.so;" \
 && git clone https://github.com/nanodbc/nanodbc.git /opt/nanodbc && mkdir -p /opt/nanodbc/build

ENV CXX g++-5
SHELL ["/bin/bash"]
