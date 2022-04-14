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

## Deploy

Push content of `doc/build/html` to `gh-pages` branch:

```console
git clone -b gh-pages https://github.com/nanodbc/nanodbc.git website
cp -a doc/build/html/* website/
cp -a doc/build/html/.buildinfo website/
cp -a doc/build/html/.nojekyl website/
cd website
git add -A
git commit -m "docs: Update build"
git push origin gh-pages
```
