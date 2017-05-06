#!/bin/bash -ue

if [[ -z ${BUILD_SHARED_LIBS+v} ]]; then
    export BUILD_SHARED_LIBS=OFF
fi

if [[ -z ${ENABLE_UNICODE+v} ]]; then
    export ENABLE_UNICODE=OFF
fi

if [[ -z ${ENABLE_BOOST+v} ]]; then
    export ENABLE_BOOST=OFF
fi

if [[ -z ${DISABLE_LIBCXX+v} ]]; then
    export DISABLE_LIBCXX=OFF
fi

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
      -DNANODBC_ENABLE_UNICODE=${ENABLE_UNICODE} \
      -DNANODBC_ENABLE_BOOST=${ENABLE_BOOST} \
      -DNANODBC_DISABLE_LIBCXX=${DISABLE_LIBCXX} \
      ..
make
cd test
make ${DB}_tests
ctest -VV --output-on-failure -R ${DB}_tests
