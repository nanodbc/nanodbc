#!/bin/bash -ue

set -o errexit
set -o pipefail
set -o nounset

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    ./utility/format.sh

    MSG="The following files have been modified:"
    dirty=$(git ls-files --modified)

    if [[ $dirty ]]; then
        echo $MSG
        echo $dirty
        # TODO: Enable hard error on formating changes
        #exit 1
    fi
fi

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

if [[ -z ${DISABLE_EXAMPLES+v} ]]; then
    export DISABLE_EXAMPLES=ON
fi

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
      -DNANODBC_ENABLE_UNICODE=${ENABLE_UNICODE} \
      -DNANODBC_ENABLE_BOOST=${ENABLE_BOOST} \
      -DNANODBC_DISABLE_LIBCXX=${DISABLE_LIBCXX} \
      -DNANODBC_DISABLE_EXAMPLES=${DISABLE_EXAMPLES} \
      ..
make
cd test
make ${DB}_tests
ctest -VV --output-on-failure -R ${DB}_tests
