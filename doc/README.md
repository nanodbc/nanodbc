# Sphinx-based docs for nanodbc

## Prerequisites

1. Python 3
2. Doxygen, which Breathe runs to read the API out of `nanodbc/nanodbc.h`
3. Node and markdownlint-cli (optional)

Doxygen is the one that does not come from pip: `brew install doxygen` on macOS, `apt-get install doxygen` on Debian and Ubuntu.

## Install

Sphinx, Breathe and the theme are listed in `requirements.txt`, which is also what the workflow installs, so a local build matches the published one.

```console
python3 -m venv doc/.venv
. doc/.venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r doc/requirements.txt
```

Optionally, install [markdownlint-cli](https://github.com/igorshubovych/markdownlint-cli).

## Lint

The reStructuredText is checked by [DOCtor-RST](https://github.com/OskarStark/doctor-rst), configured by `doc/.doctor-rst.yaml`. This is what CI runs, and it needs nothing installed here:

```console
docker run --rm -v "$PWD":/project -w /project -e DOCS_DIR=doc/ oskarstark/doctor-rst --short
```

The Markdown is checked by [markdownlint-cli](https://github.com/igorshubovych/markdownlint-cli), configured by `.markdownlint.json`, over every file rather than the README alone. Pin the version CI pins, which the `markdown-lint` action in `lint.yml` builds in: later releases added rules that the repository has never been held to, and reporting those locally would be reporting failures CI does not have.

```console
npx markdownlint-cli@0.26.0 "**/*.md"
```

## Build

The Older Versions page lists the versions the website carries, which `conf.py` reads from the `gh-pages` branch. Fetch it once so that the list can be built here; without it the page says as much instead.

```console
git fetch origin gh-pages:refs/remotes/origin/gh-pages
```

Then, with the virtualenv activated, and from the root of the repository:

```console
make -C doc clean html
open doc/build/html/index.html
```

A clean build reports three warnings, all of them Breathe failing to parse the same declaration in `api.rst`. Doxygen may add warnings of its own about `@return` on functions that return nothing, depending on its version.

## Deploy

Nothing to do by hand. Releasing deploys these pages: the [Documentation workflow](../.github/workflows/documentation.yml) commits them to the `gh-pages` branch, at the site root and under `vX.Y.Z/` as a permanent archive of that release. Nothing else writes the branch, so the published site is the documentation of the latest release. See [Publish and Release Process](../README.md#publish-and-release-process).

A pull request builds the pages as a check and attaches them as the `documentation` artifact, so a change can be reviewed as rendered HTML before it is published.
