# Sphinx-based docs for nanodbc

## Prerequisites

1. Python 3
2. Doxygen, which builds the API reference from `nanodbc/nanodbc.h`
3. Node and markdownlint-cli (optional)

Doxygen is the one that does not come from pip: `brew install doxygen` on macOS, `apt-get install doxygen` on Debian and Ubuntu.

## Install

Sphinx and the theme are listed in `requirements.txt`, which is also what the workflow installs, so a local build matches the published one.

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

The same files are also checked by [rstcheck](https://github.com/rstcheck/rstcheck), configured by `.rstcheck.cfg`. The two are complementary: DOCtor-RST enforces style conventions, while rstcheck confirms the reStructuredText actually parses. The `sphinx` extra is required, since it teaches rstcheck the roles and directives Sphinx adds, such as `:ref:` and `toctree`; without it every use of those is reported as an error. Pin the version CI pins, for the same reason as markdownlint below:

```console
python3 -m pip install "rstcheck[sphinx]==6.3.0"
rstcheck doc/*.rst
```

`.rstcheck.cfg` ignores the `release_tag` substitution, which `conf.py` defines in `rst_prolog` where rstcheck cannot see it. `report_level` keeps INFO-level notes about implicit section targets out of the way, as those are Sphinx-isms rather than defects.

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

A clean build reports no warnings, from Sphinx or from Doxygen.

The `html` target runs Sphinx first and Doxygen second, the reference living inside the output Sphinx would otherwise empty. The Doxygen step clones [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css) at the version the `Makefile` pins, so that step needs network access; `make -C doc clean` removes the clone along with the build.

## Deploy

Nothing to do by hand. Releasing deploys these pages: the [Documentation workflow](../.github/workflows/documentation.yml) commits them to the `gh-pages` branch, at the site root and under `vX.Y/` as the archive for that minor series, which a later patch refreshes rather than replaces. Nothing else writes the branch, so the published site is the documentation of the latest release. See [Publish and Release Process](../README.md#publish-and-release-process).

A pull request builds the pages as a check and attaches them as the `documentation` artifact, so a change can be reviewed as rendered HTML before it is published.
