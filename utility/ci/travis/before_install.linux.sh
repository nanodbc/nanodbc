#!/bin/bash -ue

# NOTE: To switch away from APT addon on Travis CI and
#       and install dependencies via script
# Uncomment the commands below
# Comment relevant parts in .trvis.yml
#
# sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
# sudo apt-get update -qq -y
# sudo apt-get install -qq -y \
#     clang-format-3.8 \
#     cmake \
#     cmake-data \
#     g++-5 \
#     lcov \
#     libboost-locale-dev \
#     libmyodbc \
#     libsqliteodbc \
#     mysql-client-5.6 \
#     odbc-postgresql \
#     unixodbc \
#     unixodbc-dev

sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-5 90
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 20
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
