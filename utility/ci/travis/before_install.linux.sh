#!/bin/bash -ue

# NOTE: The .travis.yml installs dependencies using the APT addon
#       available on Travis CI, but it tends to fail frequently,
#       so this ensures all dependencies are installed as required.
#       Although time overhead of this repeated installs per build job
#       is ~30 seconds only, better than randomly failing builds.
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq -y
sudo apt-get install -qq -y \
    clang-format-3.8 \
    cmake \
    cmake-data \
    g++-5 \
    lcov \
    libboost-locale-dev \
    libmyodbc \
    libsqliteodbc \
    mysql-client-5.6 \
    odbc-postgresql \
    unixodbc \
    unixodbc-dev

sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-5 90
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 20
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
