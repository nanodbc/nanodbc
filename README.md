# nanodbc

![nanodbc-banner][nanodbc-banner]

A small C++ wrapper for the native C ODBC API. Please see the [online documentation][nanodbc] for
user information, example usage, propaganda, and detailed source level documentation.

[![GitHub release](https://img.shields.io/github/tag/nanodbc/nanodbc.svg)](https://github.com/nanodbc/nanodbc/releases) [![GitHub commits](https://img.shields.io/github/commits-since/nanodbc/nanodbc/v2.14.0.svg?style=flat-square)](https://github.com/nanodbc/nanodbc/releases/tag/v2.14.0)
[![License](https://img.shields.io/github/license/nanodbc/nanodbc.svg?style=flat-square)](https://github.com/nanodbc/nanodbc/blob/main/LICENSE) [![Gitter](https://img.shields.io/gitter/room/nanodbc/nanodbc.svg?style=flat-square)](https://gitter.im/nanodbc-help/Lobby)

## Build Status

| Branch | Linux                                                                                                                                                                | Windows                                                                                                                                                                  | Coverage                                                                                                              |
| :----- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------- |
| `main` | [![main](https://github.com/nanodbc/nanodbc/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/nanodbc/nanodbc/actions/workflows/ci-linux.yml) | [![main](https://github.com/nanodbc/nanodbc/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/nanodbc/nanodbc/actions/workflows/ci-windows.yml) | [![coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fnanodbc%2Fnanodbc%2Fbadges%2Fcoverage.json)](https://github.com/nanodbc/nanodbc/actions/workflows/ci-linux.yml?query=branch%3Amain) |

Coverage is measured by the `coverage` job in [ci-linux.yml](.github/workflows/ci-linux.yml), which
instruments the library with llvm-cov and runs the utility, SQLite and PostgreSQL suites. The job
comments the figures on each pull request and prints the same report into its own summary, so the
coverage change is visible before merging, and it publishes the figure the badge above reads to the
`badges` branch. The lcov report is attached to each run as an artifact.

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

### C++ Standard

Each release line targets one C++ standard. Pick the nanodbc version that matches the standard your
project compiles with.

| nanodbc version | C++ standard |
| --------------- | ------------ |
| `< 2.12`        | C++11        |
| `>= 2.12`       | C++14        |

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

[clang-format][clang-format] handles all C++ code formatting for nanodbc. See our
[.clang-format](.clang-format) configuration file for the style, and for the major version of
`clang-format` it is written against, which is named in the comment at the top of the file. A
different major version will reformat code that is already correct, so match the one named there.
The development container carries it, and `pip install clang-format==<major>.*` supplies it on a
host.

To run `clang-format` against the whole nanodbc codebase:

```shell
clang-format -i $(git ls-files '*.h' '*.cpp')
```

[.clang-format-ignore](.clang-format-ignore) lists what to leave alone, so vendored code is
skipped even when it is named on the command line.

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

Every database nanodbc is tested against runs as a container, so none of them has to be installed on your machine. [docker-compose.yml](docker-compose.yml) defines a service per database plus a `nanodbc` development container built from [Dockerfile.dev](Dockerfile.dev), which carries a compiler toolchain, CMake, and the ODBC driver managers, drivers and client tools for all of them.

```shell
cd /path/to/nanodbc

# Start the database servers and wait until each one is accepting connections
docker compose up -d

# Open a shell in the development container, with your working tree mounted at /opt/nanodbc
docker compose run --rm nanodbc /bin/bash
```

The development container presets the `NANODBC_TEST_CONNSTR_*` variables the test programs read, so inside it a build and a full test run need no arguments:

```shell
root@hash:/opt/nanodbc# cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
root@hash:/opt/nanodbc# cmake --build build --parallel

# All of them, or one database at a time
root@hash:/opt/nanodbc# ctest --test-dir build --output-on-failure -E vertica_tests
root@hash:/opt/nanodbc# ctest --test-dir build --output-on-failure -R sqlite_tests
```

The SQLite and utility tests need no server at all, so they are the quickest way to check a change.

Each server version can be overridden, which is how a version from the CI matrix is reproduced:

```shell
POSTGRES_VERSION=14 docker compose up -d pgsql
```

MariaDB runs alongside MySQL on its own port and answers through its own ODBC driver. The MySQL
suite is the one that covers it, so point that suite at it:

```shell
docker compose run --rm \
  -e NANODBC_TEST_CONNSTR_MYSQL="$NANODBC_TEST_CONNSTR_MARIADB" nanodbc \
  ctest --test-dir build --output-on-failure -R mysql_tests
```

The Vertica tests need Vertica's own ODBC driver, which the development image does not carry, so a full `ctest` run excludes them with `-E vertica_tests`.

When you are done, `docker compose down` stops everything, and `docker compose down -v` also discards the databases' data.

To build the development image on its own, without the database services, pass the file to `docker build` with `-f`, since it is not named `Dockerfile`:

```shell
docker build -f Dockerfile.dev -t nanodbc .
docker run -v "$(pwd)":/opt/nanodbc -it nanodbc /bin/bash
```

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
locally. `docker-compose.yml` brings up the database servers for the rest, all as containers; see
[Quick Setup for Testing or Development Environments](#quick-setup-for-testing-or-development-environments).

## Publish and Release Process

Once your local `main` branch is ready for publishing (i.e. [semantic versioning][semver]), use the `utility/publish.sh` script. It bumps the major, minor, or patch version in `VERSION.txt`, adds a "Preparing" commit, and pushes that commit and the matching `vX.Y.Z` tag. For example, to make a minor update you would run `./utility/publish.sh minor`.

> **Important:** Always update [`CHANGELOG.md`](CHANGELOG.md) with information about new changes, bug fixes, and features when making a new release. `git log "v$(cat VERSION.txt)"..HEAD` lists what has landed since the last release, which is a good starting point for it. The publish script verifies that the top section of the changelog names the version being released, because that section becomes the release notes.

Pushing the tag is all it takes: [release.yml](.github/workflows/release.yml) does the rest.

1. It checks that the tag agrees with `VERSION.txt` at that commit, so a release cannot be labelled with a version the sources do not claim.
2. It publishes a GitHub release for the tag, with the notes taken from that version's section of `CHANGELOG.md` by `utility/changelog.sh`. Running the workflow again for a tag that already has a release rewrites its notes rather than failing.
3. It calls [documentation.yml](.github/workflows/documentation.yml) to build the documentation from the tag and deploy it to the `gh-pages` branch, both at the site root and under `vX.Y.Z/` as a permanent archive of that release.

The version the documentation shows comes from `VERSION.txt` by way of `doc/conf.py`, so there is no separate number to bump. The list of previous versions on the documentation front page is hand-maintained, though, so add the version that just moved into the archive to `doc/index.rst`.

Merges to `main` that touch `doc/`, `nanodbc/` or `VERSION.txt` redeploy the site root the same way, so the published documentation tracks `main` between releases. Pull requests build the documentation as a check without deploying it.

If a release needs to be driven again — the workflow failed part way through, or the notes were wrong — run it from the Actions tab with **Run workflow**, giving it the tag. That path does not move the tag.

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
