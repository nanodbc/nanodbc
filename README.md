![nanodbc-banner](https://cloud.githubusercontent.com/assets/1903876/11858632/cc0e21e6-a428-11e5-9a84-39fa27984914.png)

A small C++ wrapper for the native C ODBC API. Please see the [online documentation](http://lexicalunit.github.com/nanodbc/) for user information, example usage, propaganda, and detailed source level documentation.

## Branches

| Version | Description |
|:--- |:--- |
| `release` | Most recent published version that's deemed "stable". Review the [changelog notes](CHANGELOG.md) to see if this version is right for you. |
| `latest` | Latest published version; please use this version if CI tests are all passing. **[See all available versions.](https://github.com/lexicalunit/nanodbc/releases)** |
| `master`  | Contains the latest development code, not yet ready for a published version. |
| `v2.x.x`  | Targets C++14+. All future development will build upon this version. |
| `v1.x.x`  | Supports C++03 and optionally C++11. *There is no longer any support for this version.* |

## Build Status

| Branch | Travis CI | AppVeyor | Coverity |
|:--- |:--- |:--- |:--- |
| `master`  | [![master](https://travis-ci.org/lexicalunit/nanodbc.svg?branch=master)](https://travis-ci.org/lexicalunit/nanodbc) | [![master](https://ci.appveyor.com/api/projects/status/71nb7l794n3i8vdj/branch/master?svg=true)](https://ci.appveyor.com/project/lexicalunit/nanodbc?branch=master) | [![coverity_scan](https://scan.coverity.com/projects/7437/badge.svg)](https://scan.coverity.com/projects/lexicalunit-nanodbc) |
| `latest` | [![latest](https://travis-ci.org/lexicalunit/nanodbc.svg?branch=latest)](https://travis-ci.org/lexicalunit/nanodbc) |  [![latest](https://ci.appveyor.com/api/projects/status/71nb7l794n3i8vdj/branch/latest?svg=true)](https://ci.appveyor.com/project/lexicalunit/nanodbc?branch=latest) | |
| `release` | [![release](https://travis-ci.org/lexicalunit/nanodbc.svg?branch=release)](https://travis-ci.org/lexicalunit/nanodbc) |  [![recent](https://ci.appveyor.com/api/projects/status/71nb7l794n3i8vdj/branch/release?svg=true)](https://ci.appveyor.com/project/lexicalunit/nanodbc?branch=release) | |

> **Note:** The Coverity status uses the [coverity_scan](https://github.com/lexicalunit/nanodbc/tree/coverity_scan) branch. When `master` has had a significant amount of work pushed to it, merge those changes into `coverity_scan` as well to keep the status up to date.

# Building

nanodbc is intentionally small enough that you can drag and drop the header and implementation files into your project and run with it. For those that want it, I have also provided [CMake](http://www.cmake.org/) files which build a library object, or build and run the included tests. The CMake files will also support out of source builds.

Tests use the [Catch](https://github.com/philsquared/Catch) test framework, and CMake will automatically fetch the latest version of Catch for you at build time. To build the tests you will also need to have either unixODBC or iODBC installed and discoverable by CMake. This is easy on OS X where you can use [Homebrew](http://brew.sh/) to install unixODBC with `brew install unixodbc`, or use the system provided iODBC if you have OS X 10.9 or earlier.

The tests attempt to connect to a [SQLite](https://www.sqlite.org/) database, so you will have to have that and a SQLite ODBC driver installed. At the time of this writing, there happens to be a nice [SQLite ODBC driver](http://www.ch-werner.de/sqliteodbc/) available from Christian Werner's website, also available via Homebrew as `sqliteobdc`! The tests expect to find a data source named `sqlite` on *nix systems and `SQLite3 ODBC Driver` on Windows systems. For example, your `odbcinst.ini` file on OS X must have a section like the following.

```
[sqlite]
Description             = SQLite3 ODBC Driver
Setup                   = /usr/lib/libsqlite3odbc-0.93.dylib
Driver                  = /usr/lib/libsqlite3odbc-0.93.dylib
Threading               = 2
```

## Example Build Process

It's most convenient to create a build directory for an out of source build, but this isn't required. After you've used cmake to generate your Makefiles, `make nanodbc` will build your shared object. `make check` will build and run the tests. You can also install nanodbc to your system using `make install`.

If the tests fail, please don't hesitate to [**report it**](https://github.com/lexicalunit/nanodbc/issues/new) by creating an issue with your detailed test log (prepend your `make` command with `env CTEST_OUTPUT_ON_FAILURE=1 ` to enable verbose output please).

```shell
cd path/to/nanodbc/repository
mkdir build
cd build
cmake [Build Options] ..
make # creates shared library
make nanodbc # creates shared library
make tests # builds the tests
make test # runs the tests
make check # builds and then runs tests
make examples # builds all the example programs
make install # installs nanodbc.h and shared library
```

## Build Options

The following build options are available via [CMake command-line option](https://cmake.org/cmake/help/latest/manual/cmake.1.html) `-D`. If you are not using CMake to build nanodbc, you will need to set the corresponding `-D` compile define flags yourself. You will need to configure your build to use [boost](http://www.boost.org/) if you want to use the `NANODBC_USE_BOOST_CONVERT` option.

| CMake&nbsp;Option                     | Possible&nbsp;Values  | Default       | Details |
| ------------------------------------- | --------------------- | ------------- | ------- |
| `NANODBC_DISABLE_ASYNC`        | `OFF` or `ON`         | `OFF`         | Disables all async features. Can resolve build issues in older ODBC versions. |
| `NANODBC_ENABLE_LIBCXX`        | `OFF` or `ON`         | `ON`          | Enables usage of libc++ if found on the system. |
| `NANODBC_EXAMPLES`             | `OFF` or `ON`         | `ON`          | Enables building of examples. |
| `NANODBC_HANDLE_NODATA_BUG`    | `OFF` or `ON`         | `OFF`         | Provided to resolve issue [#33](https://github.com/lexicalunit/nanodbc/issues/33), details [in this commit](https://github.com/lexicalunit/nanodbc/commit/918d73cdf12d5903098381344eecde8e7d5d896e). |
| `NANODBC_INSTALL`              | `OFF` or `ON`         | `ON`          | Enables install target. |
| `NANODBC_ODBC_VERSION`         | `SQL_OV_ODBC3[...]`   | See Details   | **[Optional]** Sets the ODBC version macro for nanodbc to use. Default is `SQL_OV_ODBC3_80` if available, otherwise `SQL_OV_ODBC3`. |
| `NANODBC_STATIC`               | `OFF` or `ON`         | `OFF`         | Enables building a static library, otherwise the build process produces a shared library. |
| `NANODBC_TEST`                 | `OFF` or `ON`         | `ON`          | Enables tests target (alias `check`). |
| `NANODBC_USE_BOOST_CONVERT`    | `OFF` or `ON`         | `OFF`         | Provided as workaround to issue [#44](https://github.com/lexicalunit/nanodbc/issues/44). |
| `NANODBC_USE_UNICODE`          | `OFF` or `ON`         | `OFF`         | Enables full unicode support. `nanodbc::string` becomes `std::u16string` or `std::u32string`. |

## Note About iODBC

Under Windows `sizeof(wchar_t) == sizeof(SQLWCHAR) == 2`, yet on Unix systems `sizeof(wchar_t) == 4`. On unixODBC, `sizeof(SQLWCHAR) == 2` while on iODBC, `sizeof(SQLWCHAR) == sizeof(wchar_t) == 4`. This leads to incompatible ABIs between applications and drivers. If building against iODBC and the build option `NANODBC_USE_UNICODE` is `ON`, then `nanodbc::string_type` will be `std::u32string`. In **ALL** other cases it will be `std::u16string`.

Continuous integration tests run on [Travis-CI](https://travis-ci.org/). The build platform does not make available a Unicode-enabled iODBC driver. As such there is no guarantee that tests will pass in entirety on a system using iODBC. My recommendation is to use unixODBC. If you must use iODBC, consider _disabling_ unicode mode to avoid `wchar_t` issues.

---

# Contributing

## Code Style

[`clang-format`](http://clang.llvm.org/docs/ClangFormat.html) handles all C++ code formatting for nanodbc. This utility is [brew-installable](http://brew.sh/) on OS X (`brew install clang-format`) and is available on all major platforms. See our `.clang-format` configuration file for details on the style. The script `scripts/style.sh` formats all code in the repository automatically. To run `clang-format` on a single file use the following.

```shell
$ clang-format -i /path/to/file
```

**Please auto-format all code submitted in Pull Requests.**

## Publish and Release Process

Once your local `master` branch is ready for publishing (i.e. [semantic versioning](http://semver.org/)), use the `scripts/publish.sh` script. This script bumps the major, minor, or patch version, then updates the repository's `VERSION` file, adds a "Preparing" commit, and creates git tags appropriately. For example to make a minor update you would run `./scripts/publish.sh minor`.

> **Important:** Always update [`CHANGELOG.md`](CHANGELOG.md) with information about new changes, bug fixes, and features when making a new release. Use the `./scripts/changes.sh` script to aid in your composition of this document. The publish script itself will attempt to verify that the changelog file has been properly updated.

To do this manually instead, use the following steps &mdash; for example a minor update from `2.9.x` to `2.10.0`:

1. `echo "2.10.0" > VERSION`
2. `git add VERSION`
3. `git commit -m "Preparing 2.10.0 release."`
4. `git tag -f "v2.10.0"`
5. `git push -f origin "v2.10.0"`
6. `git push -f origin master:latest`

### Release Process

Release nanodbc with the `scripts/release.sh` script. All this script does is push out the `master` branch to the `release` branch, indicating that a new "stable" published version of nanodbc exists. To do so manually, execute `git push -f origin master:release`. **Caution: Do this for versions deemed "stable" based on suitable criteria.**

## Source Level Documentation

Source level documentation provided via [GitHub's gh-pages](https://help.github.com/articles/what-are-github-pages/) is available at [nanodbc.lexicalunit.com](http://lexicalunit.github.io/nanodbc/). To re-build and update it, preform the following steps from the root directory of the repository:

1. `git clone -b gh-pages git@github.com:lexicalunit/nanodbc.git doc` Necessary the first time, not subsequently.
2. `cd doc`
3. `make` Generates updated documentation locally.
4. `make commit` Adds and commits any updated documentation.
5. `git push origin gh-pages` Deploys the changes to github.

Building documentation and gh-pages requires the use of [Doxygen](www.doxygen.org) and [jekyll](https://jekyllrb.com/). See the [`Makefile` on the `gh-pages` branch](https://github.com/lexicalunit/nanodbc/blob/gh-pages/Makefile) for more details.

## Quick Setup for Testing or Development Environments

To get up and running with nanodbc as fast as possible consider using the provided [Dockerfile](Dockerfile) or [Vagrantfile](Vagrantfile). For example, to spin up a [docker](https://www.docker.com/) container suitable for testing and development of nanodbc:

```shell
$ cd /path/to/nanodbc
$ docker build -t nanodbc .
$ docker run -v "$(pwd)":"/opt/$(basename $(pwd))" -it nanodbc /bin/bash
root@hash:/# mkdir -p /opt/nanodbc/build && cd /opt/nanodbc/build
root@hash:/opt/nanodbc/build# cmake ..
root@hash:/opt/nanodbc/build# make nanodbc
```

Or, to build and ssh into a [vagrant](https://www.vagrantup.com/) VM (using VirtualBox for example) use:

```shell
$ cd /path/to/nanodbc
$ vagrant up
$ vagrant ssh
vagrant@vagrant-ubuntu-precise-64:~$ git clone https://github.com/lexicalunit/nanodbc.git
vagrant@vagrant-ubuntu-precise-64:~$ mkdir -p nanodbc/build && cd nanodbc/build
vagrant@vagrant-ubuntu-precise-64:~$ CXX=g++-5 cmake ..
vagrant@vagrant-ubuntu-precise-64:~$ make nanodbc
```

## Future work

### Good to Have / Want Someday

- Refactor tests to follow BDD pattern.
- Update codebase to use more C++14 idioms and patterns.
- Write more tests with the goal to have much higher code coverage.
- More tests for a large variety of drivers. Include performance tests.
- Clean up `bind_*` family of functions, reduce any duplication.
- Improve documentation: The main website and API docs should be more responsive.
- Provide more examples in documentation, more details, and point out any gotchas.
- Versioned generated source level API documentation for `release` and `latest`. For each major and minor published versions too?
- Add "HOWTO Build" documentation for Windows, OS X, and Linux.
