#!/bin/bash

usage()
{
    (
        echo "usage: ${0##*/}"
        echo "Print log of changes since last release. Helps when writing CHANGELOG.md."
    ) >&2
    exit 1
}

if echo "$*" | egrep -q -- "--help|-h"; then
    usage
fi

pushd "$(git rev-parse --show-toplevel)" >/dev/null
source utility/shell_control.sh

if [[ -n "$(git status -s)" ]]; then
    abort "changes exist in workspace, please commit or stash them first."
fi

version=$(cat VERSION)
tag="v$version"
run "git log ${tag}..HEAD"
