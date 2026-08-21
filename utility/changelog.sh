#!/usr/bin/env bash
#
# Print one CHANGELOG.md section, to be used as release notes. Named for a version once a
# release has been cut, and Unreleased until then.

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

# Until a release names it, what is worth reading is whatever has landed since the last one.
section="${1-Unreleased}"

cd "$(git rev-parse --show-toplevel)"

# The heading of the next version ends the section, so the body between the two headings
# is what belongs to this one.
notes="$(awk -v heading="## ${section}" '
    $0 == heading { found = 1; next }
    found && /^## / { exit }
    found { print }
' CHANGELOG.md)"

# Trailing and leading blank lines come from the spacing around the headings.
notes="$(printf '%s\n' "${notes}" | sed -e '/./,$!d' -e :a -e '/^\n*$/{$d;N;ba' -e '}')"

[[ -n "${notes}" ]] || abort "CHANGELOG.md has no content under '## ${section}'."

printf '%s\n' "${notes}"
