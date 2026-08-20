#!/usr/bin/env bash
#
# Publish a new version of nanodbc: bump VERSION.txt, commit it, and push the tag.

set -euo pipefail

usage()
{
    (
        echo "usage: ${0##*/} [major|minor|patch]"
        echo "Publish new version of nanodbc."
    ) >&2
    exit 1
}

abort()
{
    echo "error: $1" >&2
    exit 1
}

case "${1-}" in
    major | minor | patch) part="$1" ;;
    *) usage ;;
esac

cd "$(git rev-parse --show-toplevel)"

[[ -z "$(git status --porcelain)" ]] ||
    abort "changes exist in workspace, please commit or stash them first."

IFS=. read -r major minor patch < VERSION.txt

case "${part}" in
    major)
        major=$((major + 1))
        minor=0
        patch=0
        ;;
    minor)
        minor=$((minor + 1))
        patch=0
        ;;
    patch) patch=$((patch + 1)) ;;
esac

version="${major}.${minor}.${patch}"
tag="v${version}"

head -n3 CHANGELOG.md | tail -n1 | grep -qx "## ${tag}" ||
    abort "Please update CHANGELOG.md! The top version should be '${tag}'."

# The release notes are taken from this section by the Release workflow, so a section that
# is only a heading would publish an empty release.
./utility/changelog.sh "${tag}" > /dev/null

branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "${branch}" == "main" ]] ||
    abort "releases are cut from main, but the current branch is '${branch}'."

echo "Publishing nanodbc version: ${version}"
echo "${version}" > VERSION.txt
git add VERSION.txt
git commit -m "Preparing ${version} release."
git tag "${tag}"

# The branch goes first: a tag whose commit is not on main is reachable from nothing, and
# the release built from it would document a state of the sources main never had.
git push origin main
git push origin "${tag}"

echo "Pushed ${tag}. The Release workflow publishes the release and the documentation:"
echo "  https://github.com/nanodbc/nanodbc/actions/workflows/release.yml"
