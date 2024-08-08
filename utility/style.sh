#!/usr/bin/env bash
# Source, modified from: https://github.com/Project-OSRM/osrm-backend
#
# LICENSE: BSD-2-Clause
#
# Copyright (c) 2017, Project OSRM contributors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

# Get CPU count
OS="$(uname)"
NPROC="1"
if [[ $OS == "Linux" ]]; then
    NPROC=$(nproc)
elif [[ ${OS} == "Darwin" ]]; then
    NPROC=$(sysctl -n hw.physicalcpu)
fi

# Discover desired clang-format version
for config_file in .clang-format ../.clang-format ../../.clang-format; do
    if [[ -f $config_file ]]; then
        if head -n2 .clang-format | grep -oP "^#.*clang-format\s\K([0-9]+)" >/dev/null 2>&1; then
            REQUIRED_MAJOR_VERSION=$(head -n2 .clang-format | grep -oP "^#.*clang-format\s\K([0-9]+)")
            break
        fi
    fi
done
if [[ -z $REQUIRED_MAJOR_VERSION ]]; then
    echo "error: .clang-format configuration file or version comment not found"
    exit 1
fi

# Discover clang-format
if type "clang-format-$REQUIRED_MAJOR_VERSION" 2>/dev/null; then
    CLANG_FORMAT="clang-format-$REQUIRED_MAJOR_VERSION"
elif type clang-format 2>/dev/null; then
    # Clang format found, but need to check version
    CLANG_FORMAT=clang-format
    V=$(clang-format --version)
    if [[ $V != *$REQUIRED_MAJOR_VERSION* ]]; then
        echo "warning: clang-format major version is not $REQUIRED_MAJOR_VERSION (returned ${V}); formatting may be affected"
    fi
else
    echo "error: No appropriate clang-format found (expected clang-format-$REQUIRED_MAJOR_VERSION, or clang-format)"
    exit 1
fi

echo "Formatting style with ${CLANG_FORMAT} using ${NPROC} parallel jobs..."
find nanodbc test example -type f -name '*.h' -o -name '*.cpp' |
    xargs -I{} -P "${NPROC}" "${CLANG_FORMAT}" -i -style=file {}
