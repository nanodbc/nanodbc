# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrant virtual environments for nanodbc developers and users

$provision_script = <<SCRIPT
export DEBIAN_FRONTEND="noninteractive"

sudo apt-get update -qqy
sudo apt-get -qqy install \
    \$(apt-cache -q search "libboost-locale1\\..*-dev" | awk '{print $1}') \
    cmake \
    doxygen \
    g++-5 \
    git \
    libmyodbc \
    libsqliteodbc \
    make \
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

# Might not be available, but install it if it is.
sudo apt-get -y install jekyll || true

sudo odbcinst -i -d -f /usr/share/libmyodbc/odbcinst.ini
sudo odbcinst -i -d -f /usr/share/sqliteodbc/unixodbc.ini
sudo odbcinst -i -d -f /usr/share/psqlodbc/odbcinst.ini.template

sudo mysql -e "DROP DATABASE IF EXISTS nanodbc_tests; CREATE DATABASE IF NOT EXISTS nanodbc_tests;" -uroot
sudo mysql -e "GRANT ALL PRIVILEGES ON *.* TO 'root'@'localhost';" -uroot
export NANODBC_MYSQL_CONNSTR="Driver=MySQL;Server=localhost;Database=nanodbc_tests;User=root;Password=;Option=3;"
psql -c "CREATE DATABASE nanodbc_tests;" -U postgres
export NANODBC_PGSQL_CONNSTR="Driver={PostgreSQL ANSI};Server=127.0.0.1;Port=5432;Database=nanodbc_tests;UID=postgres;"

# Build in $HOME=/home/vagrant on the VM filesystem, outside of the synced /vagrant directory.
# Otherwise, CMake will fail: CMake Error: cmake_symlink_library: System Error: Protocol error
# See https://github.com/mitchellh/vagrant/issues/713
if [[ ! -d /home/vagrant/nanodbc/.git ]]; then
    mkdir -p /home/vagrant
    cd /home/vagrant
    git clone https://github.com/lexicalunit/nanodbc.git
fi
SCRIPT

# All Vagrant configuration is done below.
# For a configuration reference go to https://docs.vagrantup.com.
# The "2" in Vagrant.configure configures the configuration version.
# Please don't change it unless you know what you're doing.
Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/wily64"
  config.vm.box_check_update = true
  config.vm.provider :virtualbox do |vb|
    vb.customize ["modifyvm", :id, "--memory", " 1024"]
  end
  config.vm.provision :shell, inline: $provision_script
end
