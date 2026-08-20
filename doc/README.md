# Sphinx-based docs for nanodbc

## Prerequisites

1. Python 3
2. Sphinx
3. Breathe
4. Doxygen
5. Node and markdownlint-cli (optional)

## Install

```console
python -m venv .pyvenv
. .pyvenv/bin/activate
python -m pip install --upgrade pip
python -m pip install rstcheck
python -m pip install sphinx
python -m pip install sphinx_rtd_theme
python -m pip install breathe
```

Optionally, install markdownlint-cli

```console
curl -sL https://deb.nodesource.com/setup_12.x | sudo -E bash -
sudo apt-get install -y nodejs
npm install markdownlint-cli
./node_modules/.bin/markdownlint --version
```

## Lint

```console
rstcheck -r doc
```

```console
./node_modules/.bin/markdownlint --config .markdownlint.json README.md
```

## Build

```console
pushd doc && make clean && make html && popd
```

The Older Versions page lists the versions the website carries, which `conf.py` reads from the `gh-pages` branch. Fetch it first to build that list locally; without it the page says so instead.

```console
git fetch origin gh-pages:refs/remotes/origin/gh-pages
```

## Deploy

Nothing to do by hand. Releasing deploys these pages: the [Documentation workflow](../.github/workflows/documentation.yml) commits them to the `gh-pages` branch, at the site root and under `vX.Y.Z/` as a permanent archive of that release. Nothing else writes the branch, so the published site is the documentation of the latest release. See [Publish and Release Process](../README.md#publish-and-release-process).

A pull request builds the pages as a check and attaches them as the `documentation` artifact, so a change can be reviewed as rendered HTML before it is published.
