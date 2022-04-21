FROM ubuntu:17.04
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
 && apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys BA9EF27F \
 && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
 && curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add - \
 && add-apt-repository "$(curl -s https://packages.microsoft.com/config/ubuntu/16.04/prod.list)" \
 && apt-get -qy update && apt-get -qy install --no-upgrade --no-install-recommends \
        cmake \
        g++-5 \
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
 &&  echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
 && locale-gen \
 && odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini

# NOTE: `libmyodbc`, the package for MySQL ODBC support, is no longer available directly via a
#       simple `apt-get install libmyodbc` command. Instead, you must install it manually.
#       The following blog post provides step-by-step instructions, also used below
#       https://www.datasunrise.com/blog/how-to-install-the-mysql-odbc-driver-on-ubuntu-16-04/
# NOTE: Ubuntu 16.04 ships buggy unixODBC 2.3.1, so this container uses docker image with Ubuntu 17.04+
#       See related discussion at https://github.com/nanodbc/nanodbc/issues/149
RUN curl -SL https://dev.mysql.com/get/Downloads/Connector-ODBC/5.3/mysql-connector-odbc-5.3.9-linux-ubuntu17.04-x86-64bit.tar.gz | tar -zxC /opt \
 && cp /opt/mysql-connector-odbc-5.3.9-linux-ubuntu17.04-x86-64bit/lib/libmyodbc5* /usr/lib/x86_64-linux-gnu/odbc/ \
 && /opt/mysql-connector-odbc-5.3.9-linux-ubuntu17.04-x86-64bit/bin/myodbc-installer -d -a -n "MySQL ODBC 5.3 ANSI Driver" -t "DRIVER=/usr/lib/x86_64-linux-gnu/odbc/libmyodbc5a.so;" \
 && /opt/mysql-connector-odbc-5.3.9-linux-ubuntu17.04-x86-64bit/bin/myodbc-installer -d -a -n "MySQL ODBC 5.3 Unicode Driver" -t "DRIVER=/usr/lib/x86_64-linux-gnu/odbc/libmyodbc5w.so;" \
 && git clone https://github.com/nanodbc/nanodbc.git /opt/nanodbc && mkdir -p /opt/nanodbc/build

ENV CXX g++-5
SHELL ["/bin/bash"]
