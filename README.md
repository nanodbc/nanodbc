# nanodbc

![nanodbc-banner][nanodbc-banner]

A small C++ wrapper for the native C ODBC API. Please see the [online documentation][nanodbc] for
user information, example usage, propaganda, and detailed source level documentation.

[![GitHub release](https://img.shields.io/github/tag/nanodbc/nanodbc.svg)](https://github.com/nanodbc/nanodbc/releases) [![GitHub commits](https://img.shields.io/github/commits-since/nanodbc/nanodbc/v2.14.0.svg?style=flat-square)](https://github.com/nanodbc/nanodbc/releases/tag/v2.14.0)
[![License](https://img.shields.io/github/license/nanodbc/nanodbc.svg?style=flat-square)](https://github.com/nanodbc/nanodbc/blob/main/LICENSE) [![Gitter](https://img.shields.io/gitter/room/nanodbc/nanodbc.svg?style=flat-square)](https://gitter.im/nanodbc-help/Lobby)

## Build Status

| Branch | Linux                                                                                                                                                                | Windows                                                                                                                                                                  | Coverage                                                                                                              |
| :----- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------- |
| `main` | [![main](https://github.com/nanodbc/nanodbc/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/nanodbc/nanodbc/actions/workflows/ci-linux.yml) | [![main](https://github.com/nanodbc/nanodbc/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/nanodbc/nanodbc/actions/workflows/ci-windows.yml) | [![codecov](https://codecov.io/gh/nanodbc/nanodbc/branch/main/graph/badge.svg)](https://codecov.io/gh/nanodbc/nanodbc) |

Coverage is measured by the `coverage` job in [ci-linux.yml](.github/workflows/ci-linux.yml), which
instruments the library with llvm-cov and runs the utility, SQLite and PostgreSQL suites. The same
report is printed into the job summary of every run, so the figure is available there whether or not
the badge service is reachable.

## Philosophy

The native C API for working with ODBC is exorbitantly verbose, ridiculously complicated, and
fantastically brittle. nanodbc addresses these frustrations! The goal for nanodbc is to make
developers happy. Common tasks should be easy, requiring concise and simple code.

The [latest C++ standards][cpp-std] and [best practices][cpp-core] are
_enthusiastically_ incorporated to make the library as future-proof as possible. To accommodate
users who can not use the latest and greatest, [semantic versioning][semver] and
release notes will clarify required C++ features and/or standards for particular versions.

### Design Decisions

All complex objects in nanodbc follow the [pimpl (Pointer to IMPLementation)][pimpl] idiom to
provide separation between interface and implementation, value semantics, and a clean `nanodbc.h`
header file that includes nothing but standard C++ headers.

nanodbc wraps ODBC code, providing a simpler way to do the same thing. We try to be as featureful
as possible, but I can't guarantee you'll never have to write supporting ODBC code. Personally, I
have never had to do so.

Major features beyond what's already supported by ODBC are not within the scope of nanodbc. This is
where the _nano_ part of nanodbc becomes relevant: This library is _as minimal as possible_. That
means no dependencies beyond standard C++ and typical ODBC headers and libraries to link against.
No features unsupported by existing ODBC API calls.

## Building

nanodbc is intentionally small enough that you can drag and drop the header and implementation
files into your project and run with it. For those that want it, I have also provided
[CMake][cmake] files which build a library object, or build and run the included tests.
The CMake files will also support out of source builds.

Tests use the [Catch2][catch] test framework, vendored under `test/catch` as its amalgamated
distribution so that the tests build without network access. To build the nanodbc and the tests
you will also need to have either [unixODBC] or [iODBC] installed and discoverable by CMake.
This is easy on OS X where you can use [Homebrew][brew] to install unixODBC with `brew install unixodbc`,
or use the system provided iODBC if you have OS X 10.9 or earlier.

The tests attempt to connect to a [SQLite][sqlite] database, so you will have to have that and a
SQLite ODBC driver installed. At the time of this writing, there happens to be a nice
[SQLite ODBC driver][sqliteodbc] available from Christian Werner's website, also available via
Homebrew as `sqliteobdc`! The tests expect to find a data source named `sqlite` on \*nix systems and
`SQLite3 ODBC Driver` on Windows systems. For example, your `odbcinst.ini` file on OS X must have a
section like the following.

```ini
[sqlite]
Description             = SQLite3 ODBC Driver
Setup                   = /usr/lib/libsqlite3odbc.dylib
Driver                  = /usr/lib/libsqlite3odbc.dylib
Threading               = 2
```

### Example Build Process

It's most convenient to create a build directory for an out of source build, but this isn't
required. After you've used cmake to generate your Makefiles, `make nanodbc` will build your shared
object. `make check` will build and run the tests. You can also install nanodbc to your system
using `make install`.

If the tests fail, please don't hesitate to [**report it**][nanodbc-new-issue] by creating an issue
with your detailed test log (prepend your `make` command with `env CTEST_OUTPUT_ON_FAILURE=1` to
enable verbose output please).

```shell
cd path/to/nanodbc/repository
mkdir build
cd build
cmake [Build Options] ..
make           # creates shared library
make nanodbc   # creates shared library
make tests     # builds the tests
make test      # runs the tests
make check     # builds and then runs tests
make examples  # builds all the example programs
make install   # installs nanodbc.h and shared library
```

### Build Options

The following build options are available via [CMake command-line option][cmake-docs] `-D`. If you
are not using CMake to build nanodbc, you will need to set the corresponding `-D` compile define
flags yourself.

All boolean options follow the CMake [OPTION][cmake-option] default value convention:
if no initial value is provided, `OFF` is used.

Use the standard CMake option `-DBUILD_SHARED_LIBS=ON` to build nanodbc as shared library.

If you need to use the `NANODBC_ENABLE_BOOST=ON` option, you will have to configure your
environment to use [Boost][boost].

| CMake&nbsp;Option                  | Possible&nbsp;Values | Details |
| -----------------------------------| ---------------------| ------- |
| `NANODBC_BUILD_EXAMPLES`           | `OFF` or `ON`        | Build examples. On by default when nanodbc is the top level project. |
| `NANODBC_BUILD_TESTS`              | `OFF` or `ON`        | Build tests. On by default when nanodbc is the top level project. |
| `NANODBC_DISABLE_ASYNC`            | `OFF` or `ON`        | Disable all async features. The ODBC 3.8 async API is switched off automatically when the ODBC headers found at configure time do not declare it. |
| `NANODBC_DISABLE_MSSQL_TVP`        | `OFF` or `ON`        | Do not use MSSQL table-valued parameters. |
| `NANODBC_ENABLE_BOOST`             | `OFF` or `ON`        | Use Boost for Unicode string convertions (requires [Boost.Locale][boost-locale]). Workaround to issue [#24](https://github.com/nanodbc/nanodbc/issues/24). |
| `NANODBC_ENABLE_COVERAGE`          | `OFF` or `ON`        | Enable code coverage analysis. Requires tests to be built. |
| `NANODBC_ENABLE_UNICODE`           | `OFF` or `ON`        | Enable Unicode support. `nanodbc::string` becomes `std::u16string` or `std::u32string`. |
| `NANODBC_ENABLE_WORKAROUND_NODATA` | `OFF` or `ON`        | Enable `SQL_NO_DATA` workaround to issue [#43](https://github.com/nanodbc/nanodbc/issues/43). |
| `NANODBC_FORCE_LIBCXX`             | `OFF` or `ON`        | Force the use of libc++. On by default if the compiler supports it. |
| `NANODBC_FORCE_WARNINGS_AS_ERROR`  | `OFF` or `ON`        | Treat compiler warnings as errors when building nanodbc. |
| `NANODBC_GENERATE_INSTALL`         | `OFF` or `ON`        | Generate the install target. On by default when nanodbc is the top level project. |
| `NANODBC_OVERALLOCATE_CHAR`        | `OFF` or `ON`        | Overallocate auto-bound n/var/char buffers to accomodate retrieving Unicode data in VARCHAR columns [#219](https://github.com/nanodbc/nanodbc/issues/219). |
| `NANODBC_ODBC_VERSION`             | `SQL_OV_ODBC3[...]`  | Forces ODBC version to use. Default is `SQL_OV_ODBC3_80` if available, otherwise `SQL_OV_ODBC3`. |

### Note About iODBC

Under Windows `sizeof(wchar_t) == sizeof(SQLWCHAR) == 2`, yet on Unix systems
`sizeof(wchar_t) == 4`. On unixODBC, `sizeof(SQLWCHAR) == 2` while on iODBC,
`sizeof(SQLWCHAR) == sizeof(wchar_t) == 4`. This leads to incompatible ABIs between applications
and drivers. If building against iODBC and the build option `NANODBC_USE_UNICODE` is `ON`, then
`nanodbc::string` will be `std::u32string`. In **ALL** other cases it will be `std::u16string`.

The CI builds do not exercise a Unicode-enabled iODBC driver. As such there is no guarantee
that tests will pass in entirety on a system using iODBC. My recommendation is to use unixODBC.
If you must use iODBC, consider _disabling_ unicode mode to avoid `wchar_t` issues.

---

## Contributing

### Code Style

[clang-format][clang-format] version 15 handles all C++ code formatting for nanodbc.
See our [.clang-format](.clang-format) configuration file for details on the style and
currently required version of `clang-format` specified in the comment at the top of the file
The script [utility/style.sh](utility/style.sh) formats all code in the repository automatically.

To run `clang-format` against the whole nanodbc codebase:

```shell
./utility/style.sh
```

To run `clang-format` on a single file use the following.

```shell
clang-format -i /path/to/file
```

**Please auto-format all code submitted in Pull Requests.**

### Source Level Documentation

Source level documentation provided via [GitHub's gh-pages][gh-pages] is available
at [nanodbc.io][nanodbc]. To re-build and update it, preform the following steps
from the [doc/README.md](doc/README.md) file.

### Quick Setup for Testing or Development Environments

To get up and running with nanodbc as fast as possible consider using the provided [Dockerfile.dev](Dockerfile.dev) and [docker-compose.yml](docker-compose.yml).

`Dockerfile.dev` builds a development and testing environment: a compiler toolchain, CMake, and the ODBC driver managers, drivers and client tools for every database nanodbc is tested against. Because it is not named `Dockerfile`, pass it to `docker build` with `-f`.

For example, to spin up a [docker][docker] container suitable for testing and development of nanodbc:

```shell
$ cd /path/to/nanodbc
$ docker build -f Dockerfile.dev -t nanodbc .

# To build using the nanodbc already source within the container
$ docker run -it nanodbc /bin/bash

# Alternatively, mount the nanodbc repository into the container as a volume
$ docker run -v "$(pwd)":"/opt/$(basename $(pwd))" -it nanodbc /bin/bash

# Then, enter the source directory and build nanodbc:
root@hash:/# mkdir -p /opt/nanodbc/build
root@hash:/# cd /opt/nanodbc/build
root@hash:/opt/nanodbc/build# cmake ..
root@hash:/opt/nanodbc/build# make nanodbc
```

Or, spin up the complete multi-container environment with database services. This is the easiest way to run the database tests, since it starts PostgreSQL, MySQL and SQL Server alongside the development container, mounts your working tree at `/opt/nanodbc`, and presets the `NANODBC_TEST_CONNSTR_*` variables the test programs read:

```shell
cd /path/to/nanodbc
docker-compose build
docker-compose up -d
docker exec -it nanodbc /bin/bash

# Then, inside the container, build and run the tests:
root@hash:/# cmake -S /opt/nanodbc -B /opt/nanodbc/build -DCMAKE_BUILD_TYPE=Release
root@hash:/# cmake --build /opt/nanodbc/build

# All of them, or one database at a time
root@hash:/# ctest --test-dir /opt/nanodbc/build --output-on-failure
root@hash:/# ctest --test-dir /opt/nanodbc/build --output-on-failure -R sqlite_tests
```

The SQLite and utility tests need no server. Give the database services a few seconds to finish initializing before running the tests against them.

### Tests

One of important objectives is to maintain nanodbc covered with tests. New contributions
submitted via Pull Requests must include corresponding tests. This is important to ensure
the quality of new features.

The good news is that adding tests is easy!

The tests structure:

- `test/base_test_fixture.h` provides the helpers the fixtures share, such as connecting,
  creating and dropping tables, and reporting which backend is under test.
- `test/test_case_fixture.h` holds the test cases that run against every backend.
- `test/<database>_test.cpp` is a source code for an independent test program that includes both,
  common and database-specific test cases.

To add new test case:

1. In `test/test_case_fixture.h` file, add a new test case method to `test_case_fixture`
   class (e.g. `void my_feature_test()`).
2. In each `test/<database>_test.cpp` file, copy and paste the `TEST_CASE_METHOD` boilerplate,
   updating name, tags, etc.

If a feature requires a database-specific test case for each database, then skip the
`test/test_case_fixture.h` step and write a dedicated test case directly in
`test/<database>_test.cpp` file.

The SQLite and utility tests need no server, so they are the quickest way to check a change
locally. `docker-compose.yml` brings up PostgreSQL, MySQL and SQL Server for the rest; see
[Quick Setup for Testing or Development Environments](#quick-setup-for-testing-or-development-environments).

## Publish and Release Process

Once your local `main` branch is ready for publishing
(i.e. [semantic versioning][semver]), use the `utility/publish.sh` script. This script
bumps the major, minor, or patch version, then updates the repository's `VERSION.txt` file, adds a
"Preparing" commit, and creates git tags appropriately. For example to make a minor update you
would run `./utility/publish.sh minor`.
Review files of CMake configuration, documentation and Sphinx configuration,
and update version number wherever necessary.

> **Important:** Always update [`CHANGELOG.md`](CHANGELOG.md) with information about new changes,
> bug fixes, and features when making a new release.
> Use the `./utility/changes.sh` script to aid in your composition of this document.
> The publish script itself will attempt to verify that the changelog file has been properly updated.

To do this manually instead, use the following steps &mdash; for example a minor update from
`2.9.x` to `2.10.0`:

1. `echo "2.10.0" > VERSION.txt`
2. `git add VERSION.txt`
3. `git commit -m "Preparing 2.10.0 release."`
4. `git tag -f "v2.10.0"`
5. `git push -f origin "v2.10.0"`

Next, switch to `gh-pages` branch, build latest documentation, commit and push.

Finally, announce the new release to the public.

---

[MIT][mit] &copy; [lexicalunit, mloskot][authors] and [contributors][contributors].

[mit]: http://opensource.org/licenses/MIT
[authors]: https://github.com/orgs/nanodbc/people
[contributors]: https://github.com/nanodbc/nanodbc/graphs/contributors
[nanodbc]: http://nanodbc.io
[nanodbc-banner]: https://cloud.githubusercontent.com/assets/1903876/11858632/cc0e21e6-a428-11e5-9a84-39fa27984914.png
[nanodbc-new-issue]: https://github.com/nanodbc/nanodbc/issues/new
[boost]: http://www.boost.org/
[boost-locale]: http://www.boost.org/doc/libs/release/libs/locale/
[brew]: http://brew.sh/
[catch]: https://github.com/catchorg/Catch2
[clang-format]: http://clang.llvm.org/docs/ClangFormat.html
[cmake-docs]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[cmake]: http://www.cmake.org/
[cmake-option]: http://cmake.org/cmake/help/latest/command/option.html
[cpp-core]: https://github.com/isocpp/CppCoreGuidelines
[cpp-std]: https://isocpp.org/std/status
[docker]: https://www.docker.com/
[gh-pages]: https://help.github.com/articles/what-are-github-pages/
[iodbc]: http://www.iodbc.org/
[pimpl]: http://c2.com/cgi/wiki?PimplIdiom
[semver]: http://semver.org/
[sqlite]: https://www.sqlite.org/
[sqliteodbc]: http://www.ch-werner.de/sqliteodbc/
[unixodbc]: http://www.unixodbc.org/
