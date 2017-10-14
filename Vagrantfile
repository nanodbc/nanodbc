# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Vagrant configuration for nanodbc library development and testing
#
# Databases:
# * PostgreSQL
# * SQL Server
# * SQLite
#
# Scan the script below and fish for user/password/etc. details.
#
Vagrant.configure(2) do |config|
  config.vm.box = "bento/ubuntu-17.04"
  config.vm.network "private_network", type: "dhcp"
  config.vm.network :forwarded_port, host: 2433, guest: 1433  # SQLServer
  config.vm.network :forwarded_port, host: 6432, guest: 5432  # PostgreSQL
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "4096"
    vb.cpus = 2
  end
  config.vm.provision "shell", inline: <<-SHELL
    ############################################################################
    # Environment
    ## 'vagrant' is database user name and database name
    export DB_USER=vagrant
    export DB_PASS=vagrant
    export NANODBC_TEST_CONNSTR_MSSQL="Driver={ODBC Driver 13 for SQL Server]};Server=localhost;Database=${DB_USER};UID=${DB_USER};PWD=${DB_PASS};"
    export NANODBC_TEST_CONNSTR_PGSQL="DRIVER={PostgreSQL ANSI};Server=localhost;Database=${DB_USER};UID=${DB_USER};PWD=${DB_PASS};"
    ############################################################################
    # Packages
    export DEBIAN_FRONTEND="noninteractive"
    sudo apt-get update -y -q
    sudo apt-get upgrade -y -q
    sudo apt-get -yq --no-install-suggests --no-install-recommends install \
      cmake \
      g++-5 \
      git \
      libc6 \
      libkrb5-3 \
      libsqliteodbc \
      odbc-postgresql \
      openssl \
      postgresql \
      postgresql-client \
      postgresql-contrib \
      sqlite3 \
      unixodbc \
      unixodbc-dev \
      vim
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5
    ############################################################################
    # PostgreSQL
    echo "PostgreSQL: updating /etc/postgresql/9.6/main/postgresql.conf"
    sudo sed -i "s/#listen_address.*/listen_addresses '*'/" /etc/postgresql/9.6/main/postgresql.conf
    echo "PostgreSQL: updating /etc/postgresql/9.6/main/pg_hba.conf"
    sudo sh -c 'echo "host  all  all  0.0.0.0/0  md5" >> /etc/postgresql/9.6/main/pg_hba.conf'
    echo "PostgreSQL: creating user ${DB_USER}"
    sudo -u postgres psql -c "CREATE ROLE ${DB_USER} WITH LOGIN SUPERUSER CREATEDB ENCRYPTED PASSWORD '${DB_PASS}'"
    echo "PostgreSQL: creating database ${DB_USER}"
    sudo -u postgres dropdb --if-exists ${DB_USER}
    sudo -u postgres createdb ${DB_USER} --owner=${DB_USER}
    echo "PostgreSQL: restarting"
    sudo service postgresql restart
    ############################################################################
    # SQL Server
    export ACCEPT_EULA="Y"
    export MSSQL_PID="Developer"
    export MSSQL_SA_PASSWORD="Password123"
    echo "SQLServer: installing SQL Server from packages.microsoft.com"
    curl -s https://packages.microsoft.com/keys/microsoft.asc | sudo apt-key add -
    sudo add-apt-repository "$(curl -s https://packages.microsoft.com/config/ubuntu/16.04/mssql-server.list)"
    sudo add-apt-repository "$(curl -s https://packages.microsoft.com/config/ubuntu/16.04/prod.list)"
    sudo apt-get update -y -q
    sudo -E bash -c 'apt-get -y -q install mssql-server'
    sudo -E bash -c 'apt-get -y -q install mssql-tools'
    echo "SQLServer: running /opt/mssql/bin/mssql-conf -n setup"
    echo "SQLServer: MSSQL_PID=$MSSQL_PID"
    echo "SQLServer: MSSQL_SA_PASSWORD=$MSSQL_SA_PASSWORD"
    sudo -E bash -c '/opt/mssql/bin/mssql-conf -n setup'
    sudo /opt/mssql/bin/mssql-conf set telemetry.customerfeedback false
    echo "SQLServer: restarting service"
    sudo systemctl stop mssql-server
    sudo systemctl start mssql-server
    sudo systemctl status mssql-server
    echo "SQLServer: adding /opt/mssql-tools/bin to PATH in ~/.bashrc and ~/.bash_profile"
    echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bash_profile
    echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bashrc
    echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> /home/vagrant/.bash_profile
    echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> /home/vagrant/.bashrc
    echo "SQLServer: Running sqlcmd -Q SELECT @@version"
    export PATH="$PATH:/opt/mssql-tools/bin"
    echo "SQLServer: Creating user ${DB_USER}"
    sqlcmd -S localhost -U SA -P "${MSSQL_SA_PASSWORD}" -Q "CREATE LOGIN ${DB_USER} WITH PASSWORD='${DB_PASS}', CHECK_POLICY=OFF"
    sqlcmd -S localhost -U SA -P "${MSSQL_SA_PASSWORD}" -Q "CREATE USER ${DB_USER} FOR LOGIN ${DB_USER}"
    sqlcmd -S localhost -U SA -P "${MSSQL_SA_PASSWORD}" -Q "GRANT CREATE ANY DATABASE TO ${DB_USER}"
    echo "SQLServer: creating user ${DB_USER}"
    sqlcmd -S localhost -U ${DB_USER} -P ${DB_PASS} -Q "CREATE DATABASE ${DB_USER}"
    sqlcmd -S localhost -U ${DB_USER} -P ${DB_PASS} -d ${DB_USER} -Q "SELECT @@version;"
    ############################################################################
    # Clean up
    sudo apt-get -y -q autoremove
    sudo apt-get -y -q clean
    ############################################################################
    # ODBC
    echo "ODBC: Installed drivers:"
    odbcinst -q -d
    ############################################################################
    # Build in /home/vagrant, but not in the shared /vagrant where CMake will fail)
    echo "Cloning nanodbc into /home/vagrant"
    cd /home/vagrant
    git clone https://github.com/nanodbc/nanodbc.git
    sudo chown -R vagrant:vagrant /home/vagrant/nanodbc
    ############################################################################
    echo "Guest IP address:"
    ip addr show|grep -w inet
  SHELL
end
