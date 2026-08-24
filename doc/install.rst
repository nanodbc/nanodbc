.. _install:

##############################################################################
Install
##############################################################################

nanodbc is distributed in form of source code package.

nanodbc is intentionally small enough that you can drag and drop the header and implementation files into your project and run with it.

Binary packages, if available, are provided and supported by third-parties, developers and maintainers of numerous package managers.

******************************************************************************
Source
******************************************************************************

You can build nanodbc library, build and run tests using `CMake`_.

.. code-block:: console

  git clone https://github.com/nanodbc/nanodbc.git
  cd nanodbc
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel
  ctest --test-dir build --output-on-failure -E vertica_tests

The tests connect to real databases, so see :ref:`Test <test>` below for what each of them needs.

C++ Standard
==============================================================================

Each release line sets a minimum C++ standard. Pick the nanodbc version whose minimum your project can meet; anything newer than the minimum also works.

.. list-table::
   :header-rows: 1
   :widths: 30 30

   * - nanodbc version
     - Minimum C++ standard
   * - ``< 2.12``
     - C++11
   * - ``2.12`` - ``2.x``
     - C++14
   * - ``>= 3.0``
     - C++17

CI builds every supported compiler against C++17 and C++20, so both are exercised on each change.

Requirements
==============================================================================

* C++ compiler with support for the standard the nanodbc version targets, see above
* `CMake`_ 3.21.0 or later
* ODBC SDK (`unixODBC`_, `iODBC`_, Windows SDK)

Optionally, you will also need:

* ODBC drivers, depending on DBMS you want to target (eg. running tests).
* `Boost.Locale`_, alternative for Unicode conversion routines.
* `libc++`_, alternative C++ standard library implementation.

.. _build:

Build
==============================================================================

Although detailed build process depends on CMake generator used, number of common targets are always available.

For example, CMake configuration using Makefiles generator:

.. code-block:: console

  cd nanodbc
  mkdir build
  cd build
  cmake -G "Unix Makefiles" [options] ..
  make          # builds the library, the tests and the examples
  make nanodbc  # builds the library alone
  make tests    # builds the tests
  make test     # runs the tests
  make examples # builds all the example programs
  make install  # installs nanodbc.h and the library

The library is static unless ``BUILD_SHARED_LIBS`` is ``ON``. Tests and examples are built by default only when nanodbc is the top level project.

`CMake OPTION`_ and cache entry variables are available to specify with ``-D`` switch to enable or disable nanodbc built-in features.

All boolean options follow the `CMake OPTION`_ default value convention: if no initial value is provided, `OFF` is used.

List of CMake options specific to nanodbc, in alphabetical order:

NANODBC_BUILD_EXAMPLES : *boolean*
    Build examples. On by default when nanodbc is the top level project.

NANODBC_BUILD_TESTS : *boolean*
    Build tests. On by default when nanodbc is the top level project.

NANODBC_DISABLE_ASYNC : *boolean*
    Disable all async features. The ODBC 3.8 async API is switched off automatically when the ODBC headers found at configure time do not declare it, so this is only needed to turn it off against headers that do.

NANODBC_DISABLE_MSSQL_TVP : *boolean*
    Do not use MSSQL table-valued parameters.

NANODBC_ENABLE_BOOST : *boolean*
    Use Boost for Unicode string conversions (requires `Boost.Locale`_ and ``NANODBC_ENABLE_UNICODE=ON``). Workaround to issue `#24 <https://github.com/nanodbc/nanodbc/issues/24>`_.

NANODBC_ENABLE_COVERAGE : *boolean*
    Enable code coverage analysis. Requires tests to be built.

NANODBC_ENABLE_UNICODE : *boolean*
    Enable Unicode support. ``nanodbc::string`` becomes ``std::u16string`` or ``std::u32string``.

NANODBC_ENABLE_WORKAROUND_NODATA : *boolean*
    Enable ``SQL_NO_DATA`` workaround `#43 <https://github.com/nanodbc/nanodbc/issues/43>`_.

NANODBC_FORCE_LIBCXX : *boolean*
    Force the use of libc++. On by default if the compiler supports it.

NANODBC_FORCE_WARNINGS_AS_ERROR : *boolean*
    Treat compiler warnings as errors when building nanodbc.

NANODBC_GENERATE_INSTALL : *boolean*
    Generate the install target. On by default when nanodbc is the top level project.

NANODBC_ODBC_VERSION : *string*
    Forces ODBC version to use. Default is ``SQL_OV_ODBC3_80`` if available, otherwise ``SQL_OV_ODBC3``.

NANODBC_OVERALLOCATE_CHAR : *boolean*
    Overallocate auto-bound n/var/char buffers to accommodate retrieving Unicode data in VARCHAR columns (requires ``NANODBC_ENABLE_UNICODE=ON``) `#219 <https://github.com/nanodbc/nanodbc/issues/219>`_.


Standard `CMake`_ options are also available, for example:

BUILD_SHARED_LIBS : *boolean*
    Build nanodbc as a shared library. Default value is ``OFF``.

If you are not using CMake to build nanodbc, you will need to set the options, using the corresponding names, as preprocessor defines yourself.

.. _test:

Test
==============================================================================

Tests use the `Catch2`_ test framework, vendored under ``test/catch`` as its amalgamated distribution, so the tests build without network access.

Once nanodbc build is ready, use `ctest`_ to run tests in CMake generator-agnostic way:

.. code-block:: console

  ctest --test-dir build --output-on-failure -E vertica_tests

Alternatively, build the ``test`` target (eg. ``make test``).

There is one test program per database, so a single suite can be run on its own:

.. code-block:: console

  ctest --test-dir build --output-on-failure -R sqlite_tests

The utility tests need no database at all, and the SQLite tests need only a SQLite ODBC driver, registered as ``SQLite3`` on \*nix systems and as ``SQLite3 ODBC Driver`` on Windows, since the tests name the driver rather than a data source. Those two are the quickest way to check a build. The remaining suites need a running server, and the Vertica tests additionally need Vertica's own ODBC driver, which is why a full run excludes them.

Each suite reads its own ``NANODBC_TEST_CONNSTR_<DB>`` environment variable for the connection string, falling back to ``NANODBC_TEST_CONNSTR``. Rather than installing the servers, use the containers the repository provides, which preset those variables; see :ref:`Develop <develop>`.

******************************************************************************
Binaries
******************************************************************************

This section aim to list all known binary packages of nanodbc.

If you maintain binary package of nanodbc and you'd like to list it here, please submit new entry via pull request or `open an issue <https://github.com/nanodbc/nanodbc/issues/new>`_

Windows
==============================================================================

* vcpkg `port of nanodbc <https://github.com/Microsoft/vcpkg/tree/master/ports/nanodbc>`_

.. _`CMake`: https://cmake.org
.. _`CMake OPTION`: https://cmake.org/cmake/help/latest/command/option.html
.. _`Catch2`: https://github.com/catchorg/Catch2
.. _`ctest`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
.. _`iODBC`: http://www.iodbc.org
.. _`unixODBC`: http://www.unixodbc.org
.. _`Boost.Locale`: https://www.boost.org/doc/libs/release/libs/locale/
.. _`libc++`: https://libcxx.llvm.org
