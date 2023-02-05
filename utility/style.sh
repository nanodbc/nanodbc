#!/usr/bin/env bash
# Source https://github.com/Project-OSRM/osrm-backend

set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

# Get CPU count
OS=$(uname)
NPROC=1
if [[ $OS = "Linux" ]] ; then
    NPROC=$(nproc)
elif [[ ${OS} = "Darwin" ]] ; then
    NPROC=$(sysctl -n hw.physicalcpu)
fi

# Discover desired clang-format version
for config_file in .clang-format ../.clang-format ../../.clang-format
do
    if [ -f "$config_file" ]; then
        if head -n2 .clang-format | grep -oP "^#.*clang-format\s\K([0-9]+)" > /dev/null 2>&1; then
            REQUIRED_MAJOR_VERSION=$(head -n2 .clang-format | grep -oP "^#.*clang-format\s\K([0-9]+)")
            break
        fi
    fi
done
if [ -z "$REQUIRED_MAJOR_VERSION" ]; then
    echo ".clang-format configuration file or version comment not found"
    exit 1
fi

# Discover clang-format
if type "clang-format-${REQUIRED_MAJOR_VERSION}" 2> /dev/null ; then
    CLANG_FORMAT=clang-format-${REQUIRED_MAJOR_VERSION}
elif type clang-format 2> /dev/null ; then
    # Clang format found, but need to check version
    CLANG_FORMAT=clang-format
    MAJOR_VERSION=$(clang-format --version | cut -d' ' -f3 | cut -d'.' -f1)
    if [[ "$MAJOR_VERSION" != "$REQUIRED_MAJOR_VERSION" ]] ; then
        echo "clang-format is not required $REQUIRED_MAJOR_VERSION but ${MAJOR_VERSION}"
        exit 1
    fi
else
    echo "No appropriate clang-format found (expected clang-format-${REQUIRED_MAJOR_VERSION}, or clang-format)"
    exit 1
fi

find nanodbc test example -type f -name '*.h' -o -name '*.cpp' \
  | xargs -I{} -P ${NPROC} ${CLANG_FORMAT} -i -style=file {}
