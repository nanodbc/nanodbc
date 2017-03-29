#!/bin/bash -ue

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
      -DNANODBC_USE_UNICODE=${USE_UNICODE} \
      -DNANODBC_USE_BOOST_CONVERT=${USE_BOOST_CONVERT} \
      -DNANODBC_ENABLE_LIBCXX=${ENABLE_LIBCXX} \
      ..
make
cd test
make ${DB}_tests
ctest -VV --output-on-failure -R ${DB}_tests
