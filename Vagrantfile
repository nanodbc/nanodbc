# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/wily64"
  config.vm.network :forwarded_port, guest: 3306, host: 3306, auto_correct: true
  config.vm.network :forwarded_port, guest: 5432, host: 5432, auto_correct: true
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "1024"
    vb.cpus = 2
  end
  config.vm.network "private_network", type: "dhcp"
  config.vm.provision "shell", inline: <<-SHELL
    ############################################################################
    # Environment
    ## 'vagrant' is database user name and database name
    export DB_USER=vagrant
    export DB_PASS=vagrant
    export MYSQL_CONNSTR="Driver={MySQL};Server=localhost;Database=${DB_USER};User=${DB_USER};Password=${DB_PASS};Option=3;"
    export PGSQL_CONNSTR="DRIVER={PostgreSQL ANSI};Server=localhost;Database=${DB_USER};UID=${DB_USER};PWD=${DB_PASS};"
    ############################################################################
    # Packages
    export DEBIAN_FRONTEND="noninteractive"
    sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${DB_PASS}"
    sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${DB_PASS}"
    sudo apt-get update -y -q
    sudo apt-get -yq --no-install-suggests --no-install-recommends --force-yes -o Dpkg::Options::='--force-confnew' install \
      cmake \
      e2fsprogs \
      g++-5 \
      git \
      libc6 \
      libkrb5-3 \
      libmyodbc \
      libsqliteodbc \
      mysql-client \
      mysql-server \
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
    # MySQL
    echo "MySQL: updating /etc/mysql/mysql.conf.d/mysqld.cnf"
    sudo sed -i "s/bind-address.*/bind-address = 0.0.0.0/" /etc/mysql/mysql.conf.d/mysqld.cnf
    echo "MySQL: setting root password to ${DB_PASS}"
    mysql -uroot -p${DB_PASS} -e \
      "GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '${DB_PASS}' WITH GRANT OPTION; FLUSH PRIVILEGES;"
    echo "MySQL: creating user ${DB_USER}"
    mysql -uroot -p${DB_PASS} -e \
      "GRANT ALL PRIVILEGES ON *.* TO '${DB_USER}'@'%' IDENTIFIED BY '${DB_PASS}' WITH GRANT OPTION"
    mysql -uroot -p${DB_PASS} -e "DROP DATABASE IF EXISTS ${DB_USER}"
    echo "MySQL: creating database ${DB_USER}"
    mysql -uroot -p${DB_PASS} -e "CREATE DATABASE ${DB_USER}"
    echo "MySQL: restarting"
    sudo service mysql restart
    ############################################################################
    # PostgreSQL
    echo "PostgreSQL: updating /etc/postgresql/9.4/main/postgresql.conf"
    sudo sed -i "s/#listen_address.*/listen_addresses '*'/" /etc/postgresql/9.4/main/postgresql.conf
    echo "PostgreSQL: updating /etc/postgresql/9.4/main/pg_hba.conf"
    sudo sh -c 'echo "host  all  all  0.0.0.0/0  md5" >> /etc/postgresql/9.4/main/pg_hba.conf'
    echo "PostgreSQL: creating user ${DB_USER}"
    sudo -u postgres psql -c "CREATE ROLE ${DB_USER} WITH LOGIN SUPERUSER CREATEDB ENCRYPTED PASSWORD '${DB_PASS}'"
    echo "PostgreSQL: creating database ${DB_USER}"
    sudo -u postgres dropdb --if-exists ${DB_USER}
    sudo -u postgres createdb ${DB_USER} --owner=${DB_USER}
    echo "PostgreSQL: restarting"
    sudo service postgresql restart
    ############################################################################
    # SQL Server
    echo "SQLServer: downloading msodbcsql-11.0.2270.0.tar.gz"
    wget http://download.microsoft.com/download/B/C/D/BCDD264C-7517-4B7D-8159-C99FC5535680/RedHat6/msodbcsql-11.0.2270.0.tar.gz
    tar -zxvf msodbcsql-11.0.2270.0.tar.gz
    cd msodbcsql-11.0.2270.0
    rm install.sh build_dm.sh
    echo "SQLServer: downloading https://raw.githubusercontent.com/tax/mssqldriver/master/install.sh"
    wget https://raw.githubusercontent.com/tax/mssqldriver/master/install.sh
    echo "SQLServer: installing msodbcsql-11.0.2270.0.tar.gz"
    sudo bash install.sh install --accept-license --force
    # in case install.sh fails to create these two symlinks
    sudo ln -s /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 /usr/lib/x86_64-linux-gnu/libcrypto.so.10
    sudo ln -s /lib/x86_64-linux-gnu/libssl.so.1.0.0 /usr/lib/x86_64-linux-gnu/libssl.so.10
    cd /home/vagrant
    rm -rf /home/vagrant/msodbcsql-11.0.2270.0
    echo "SQLServer: try 'sqlcmd -S my.sql.server.com -U username -P password'"
    ############################################################################
    # ODBC
    echo "ODBC: Installed drivers:"
    odbcinst -q -d
    ############################################################################
    # Build in /home/vagrant, but not in the shared /vagrant where CMake will fail)
    echo "Cloning nanodbc into /home/vagrant"
    cd /home/vagrant
    git clone https://github.com/lexicalunit/nanodbc.git
    sudo chown -R vagrant:vagrant /home/vagrant/nanodbc
    ############################################################################
    echo "Guest IP address:"
    /sbin/ifconfig | grep 'inet addr:'
  SHELL
end
