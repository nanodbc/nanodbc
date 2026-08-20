#!/usr/bin/env bash
#
# Print the CHANGELOG.md section for one version, to be used as its release notes.

set -euo pipefail

usage()
{
    (
        echo "usage: ${0##*/} <tag>"
        echo "Print the CHANGELOG.md section for the given version tag, e.g. v2.16.0."
    ) >&2
    exit 1
}

abort()
{
    echo "error: $1" >&2
    exit 1
}

tag="${1-}"
[[ -n "${tag}" ]] || usage

cd "$(git rev-parse --show-toplevel)"

# The heading of the next version ends the section, so the body between the two headings
# is what belongs to this one.
notes="$(awk -v tag="## ${tag}" '
    $0 == tag { found = 1; next }
    found && /^## / { exit }
    found { print }
' CHANGELOG.md)"

# Trailing and leading blank lines come from the spacing around the headings.
notes="$(printf '%s\n' "${notes}" | sed -e '/./,$!d' -e :a -e '/^\n*$/{$d;N;ba' -e '}')"

[[ -n "${notes}" ]] || abort "CHANGELOG.md has no section for '${tag}'."

printf '%s\n' "${notes}"
