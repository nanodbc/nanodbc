#!/bin/bash

usage()
{
    (
        echo "usage: ${0##*/}"
        echo "Use clang-format to enforce coding style."
    ) >&2
    exit 1
}

if echo "$*" | grep -E -q -- "--help|-h"; then
    usage
fi

pushd "$(git rev-parse --show-toplevel)" >/dev/null || exit 1

for I in nanodbc/*.{h,cpp}; do
    clang-format -i "$I"
done

for I in test/*.{h,cpp}; do
    clang-format -i "$I"
done

for I in example/*.{h,cpp}; do
    clang-format -i "$I"
done
