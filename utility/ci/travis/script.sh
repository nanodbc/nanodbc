#!/bin/bash -ue

set -o errexit
set -o pipefail
set -o nounset

# Since Linux build job with sole task to verify code formatting
if [ ! -z ${CLANGFORMAT+x} ] && [ "$CLANGFORMAT" == "ON" ]; then
    ./utility/format.sh

    dirty=$(git ls-files --modified)

    if [[ $dirty ]]; then
        echo "Files with unexpected source code formatting:"
        echo $dirty
        exit 1
    else
        echo "All files verified for expected source code formatting"
        exit 0
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

if [[ -z ${ENABLE_COVERAGE+v} ]] || [[ ! -z ${COVERITY+v} ]] || [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    export ENABLE_COVERAGE=OFF
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
      -DNANODBC_ENABLE_COVERAGE=${ENABLE_COVERAGE} \
      -DNANODBC_ENABLE_UNICODE=${ENABLE_UNICODE} \
      -DNANODBC_ENABLE_BOOST=${ENABLE_BOOST} \
      -DNANODBC_DISABLE_LIBCXX=${DISABLE_LIBCXX} \
      -DNANODBC_DISABLE_EXAMPLES=${DISABLE_EXAMPLES} \
      ..
make
cd test
make utility_tests
ctest -VV --output-on-failure -R utility_tests
make ${DB}_tests
ctest -VV --output-on-failure -R ${DB}_tests
