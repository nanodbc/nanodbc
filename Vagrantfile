# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "1024"
  end
  config.vm.network "public_network", type: "dhcp"
  config.vm.provision "shell", inline: <<-SHELL
    ############################################################################
    # Environment
    export DB_USER=vagrant # used as database user name and database name
    export DB_PASS=vagrant
    export MYSQL_CONNSTR="Driver={MySQL};Server=localhost;Database=${DB_USER};User=${DB_USER};Password=${DB_PASS};Option=3;"
    export PGSQL_CONNSTR="DRIVER={PostgreSQL ANSI};Server=localhost;Database=${DB_USER};UID=${DB_USER};PWD=${DB_PASS};"
    ############################################################################
    # Packages
    export DEBIAN_FRONTEND="noninteractive"
    sudo debconf-set-selections <<< "mysql-server mysql-server/root_password password ${DB_PASS}"
    sudo debconf-set-selections <<< "mysql-server mysql-server/root_password_again password ${DB_PASS}"
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -y -q
    sudo apt-get -o Dpkg::Options::='--force-confnew' -y -q install \
      cmake \
      g++-5 \
      git \
      libmyodbc \
      libsqliteodbc \
      mysql-client \
      mysql-server \
      odbc-postgresql \
      postgresql \
      postgresql-client \
      postgresql-contrib \
      sqlite3 \
      unixodbc \
      unixodbc-dev \
      vim
    ############################################################################
    # MySQL
    echo "MySQL: updating /etc/mysql/my.cnf"
    sudo sed -i "s/bind-address.*/bind-address = 0.0.0.0/" /etc/mysql/my.cnf
    echo "MySQL: setting root password to ${DB_PASS}"
    mysql -uroot -p${DB_PASS} -e \
      "GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '${DB_PASS}' WITH GRANT OPTION; FLUSH PRIVILEGES;"
    echo "MySQL: creating user ${DB_USER}"
    mysql -uroot -p${DB_PASS} -e \
      "GRANT ALL PRIVILEGES ON ${DB_USER}.* TO '${DB_USER}'@'%' IDENTIFIED BY '${DB_PASS}' WITH GRANT OPTION"
    mysql -uroot -p${DB_PASS} -e "DROP DATABASE IF EXISTS ${DB_USER}"
    echo "MySQL: creating database ${DB_USER}"
    mysql -uroot -p${DB_PASS} -e "CREATE DATABASE ${DB_USER}"
    echo "MySQL: restarting"
    sudo service mysql restart
    ############################################################################
    # PostgreSQL
    echo "PostgreSQL: updating /etc/postgresql/9.3/main/postgresql.conf"
    sudo sed -i "s/#listen_address.*/listen_addresses '*'/" /etc/postgresql/9.3/main/postgresql.conf
    echo "PostgreSQL: updating /etc/postgresql/9.3/main/pg_hba.conf"
    sudo sh -c 'echo "host  all  all  0.0.0.0/0  md5" >> /etc/postgresql/9.3/main/pg_hba.conf'
    echo "PostgreSQL: creating user ${DB_USER}"
    sudo -u postgres psql -c "CREATE ROLE ${DB_USER} WITH LOGIN SUPERUSER CREATEDB ENCRYPTED PASSWORD '${DB_PASS}'"
    echo "PostgreSQL: creating database ${DB_USER}"
    sudo -u postgres dropdb --if-exists ${DB_USER}
    sudo -u postgres createdb ${DB_USER} --owner=${DB_USER}
    echo "PostgreSQL: restarting"
    sudo service postgresql restart
    ############################################################################
    # Build in /home/vagrant, but not in the shared /vagrant where CMake will fail)  
    echo "Cloning nanodbc into /home/vagrant"
    cd /home/vagrant
    git clone https://github.com/lexicalunit/nanodbc.git
    ############################################################################
    echo "Guest IP address:"
    /sbin/ifconfig | grep 'inet addr:'
  SHELL
end
