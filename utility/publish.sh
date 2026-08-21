#!/usr/bin/env bash
#
# Publish a new version of nanodbc.

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

head -n3 CHANGELOG.md | tail -n1 | grep -qx "## Unreleased" ||
    abort "CHANGELOG.md must open with an '## Unreleased' section for ${tag} to be named after."

./utility/changelog.sh Unreleased > /dev/null

branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "${branch}" == "main" ]] ||
    abort "releases are cut from main, but the current branch is '${branch}'."

echo "Publishing nanodbc version: ${version}"
echo "${version}" > VERSION.txt

awk -v heading="## ${tag}" '
    !renamed && $0 == "## Unreleased" { print heading; renamed = 1; next }
    { print }
' CHANGELOG.md > CHANGELOG.md.new
mv CHANGELOG.md.new CHANGELOG.md

git add VERSION.txt CHANGELOG.md
git commit -m "Preparing ${version} release."
git tag "${tag}"

git push origin main
git push origin "${tag}"

echo "Pushed ${tag}. The Release workflow publishes the release and the documentation:"
echo "  https://github.com/nanodbc/nanodbc/actions/workflows/release.yml"
