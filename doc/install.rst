.. _install:

##############################################################################
Install
##############################################################################

nanodbc is distributed in form of source code package.

nanodbc is intentionally small enough that you can drag and drop the header
and implementation files into your project and run with it.

Binary packages, if available, are provided and supported by third-parties,
developers and maintainers of numerous package managers.

******************************************************************************
Source
******************************************************************************

You can build nanodbc library, build and run tests using `CMake`_.

.. code-block:: console

  git clone https://github.com/nanodbc/nanodbc.git
  cd nanodbc
  mkdir build
  cd build
  cmake ..
  cmake --build .
  ctest -V --output-on-failure

Requirements
==============================================================================

* C++ compiler with C++11/C++14 support
* `CMake`_ 3.0.0 or later
* ODBC SDK (`unixODBC`_, `iODBC`_, Windows SDK)

Optionally, you will also need:

* ODBC drivers, depending on DBMS you want to target (eg. running tests).
* `Boost.Locale`_, alternative for Unicode conversion routines.
* `libc++`_, alternative C++11 and later implementation.

.. _build:

Build
==============================================================================

Although detailed build process depends on CMake generator used,
number of common targets are always available.

For example, CMake configuration using Makefiles generator:

.. code-block:: console

  cd nanodbc
  mkdir build
  cd build
  cmake -G "Unix Makefiles" [options] ..
  make          # creates shared library
  make nanodbc  # creates shared library
  make tests    # builds the tests
  make test     # runs the tests
  make check    # builds and then runs tests
  make examples # builds all the example programs
  make install  # installs nanodbc.h and shared library

`CMake OPTION`_ and cache entry variables are available to specify
with ``-D`` switch to enable or disable nanodbc built-in features.

All boolean options follow the `CMake OPTION`_ default value convention:
if no initial value is provided, `OFF` is used.

List of CMake options specific to nanodbc, in alphabetical order:

NANODBC_DISABLE_ASYNC : *boolean*
    Disable all async features. May resolve build issues in older ODBC versions.

NANODBC_DISABLE_EXAMPLES : *boolean*
    Do not build examples.

NANODBC_DISABLE_INSTALL : *boolean*
    Do not generate install target.

NANODBC_DISABLE_LIBCXX : *boolean*
    Do not use libc++, if available on the system.

NANODBC_DISABLE_TESTS : *boolean*
    Do not build tests. Do not generate ``test`` and ``check`` targets.

NANODBC_ENABLE_BOOST : *boolean*
    Use Boost for Unicode string convertions (requires `Boost.Locale`_). Workaround to issue `#44 <https://github.com/lexicalunit/nanodbc/issues/44>`_.

NANODBC_ENABLE_UNICODE : *boolean*
    Enable Unicode support. ``nanodbc::string`` becomes ``std::u16string`` or ``std::u32string``.

NANODBC_ENABLE_WORKAROUND_NODATA : *boolean*
    Enable ``SQL_NO_DATA`` workaround `#33 <https://github.com/lexicalunit/nanodbc/issues/33>`_.

NANODBC_ODBC_VERSION : *string*
    Force ODBC version to use.
    Possible values are ``SQL_OV_ODBC3`` or ``SQL_OV_ODBC3_80``.
    Default value is ``SQL_OV_ODBC3_80``, if available.


Standard `CMake`_ options are also available, for example:

BUILD_SHARED_LIBS : boolean
    Build nanodbc as A shared library.
    Default value is OFF.

If you are not using CMake to build nanodbc, you will need to set the options,
using the corresponding names, as preprocessor defines yourself.

Test
==============================================================================

Tests use the `Catch <https://github.com/philsquared/Catch>`_ test framework.
CMake automatically fetches the latest version of Catch for you at build time.

Once nanodbc build is ready, use `ctest`_ to run tests in
CMake generator-agnostic way:

.. code-block:: console

  ctest -V --output-on-failure

Alternatively, build `test` target (eg. `make test`).

******************************************************************************
Binaries
******************************************************************************

This section aim to list all known binary packages of nanodbc.

If you maintain binary package of nanodbc and you'd like to list it here,
please submit new entry via pull request or
`open an issue <https://github.com/nanodbc/nanodbc/issues/new>`_

Windows
==============================================================================

* vcpkg `port of nanodbc <https://github.com/Microsoft/vcpkg/tree/master/ports/nanodbc>`_

.. _`CMake`: https://cmake.org
.. _`CMake OPTION`: https://cmake.org/cmake/help/latest/command/option.html
.. _`ctest`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
.. _`iODBC`: http://www.iodbc.org
.. _`unixODBC`: http://www.unixodbc.org
.. _`Boost.Locale`: https://www.boost.org/doc/libs/release/libs/locale/
.. _`libc++`: https://libcxx.llvm.org
