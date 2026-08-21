#!/usr/bin/env bash
#
# Print one CHANGELOG.md section, to be used as release notes.

set -euo pipefail

usage()
{
    (
        echo "usage: ${0##*/} [section]"
        echo "Print the CHANGELOG.md section for the given heading, e.g. v2.16.0."
        echo "Defaults to Unreleased, the section a release has not yet named."
    ) >&2
    exit 1
}

abort()
{
    echo "error: $1" >&2
    exit 1
}

case "${1-}" in
    -h | --help) usage ;;
esac

section="${1-Unreleased}"

cd "$(git rev-parse --show-toplevel)"

notes="$(awk -v heading="## ${section}" '
    $0 == heading { found = 1; next }
    found && /^## / { exit }
    found { print }
' CHANGELOG.md)"

notes="$(printf '%s\n' "${notes}" | sed -e '/./,$!d' -e :a -e '/^\n*$/{$d;N;ba' -e '}')"

[[ -n "${notes}" ]] || abort "CHANGELOG.md has no content under '## ${section}'."

printf '%s\n' "${notes}"
