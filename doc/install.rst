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

  git clone https://github.com/lexicalunit/nanodbc.git
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
* ODBC SDK (unixODBC, iODBC, Windows SDK)

Optionally, you will also need:

* ODBC drivers, depending on DBMS you want to target (eg. running tests).
* Boost, `Boost.Locale`_ as alternative for Unicode conversions.
* `libc++`_, alternative C++11 and later implementation.

Building
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

CMake Options
------------------------------------------------------------------------------

The following build options are available via CMake command-line
option ``-D``.

Use the standard CMake option ``-DBUILD_SHARED_LIBS=ON`` to build nanodbc
as shared library.

List of nanodbc-specific options

* ``NANODBC_DISABLE_ASYNC`` - ``ON|OFF``, default is ``OFF``.
  Disables all async features. Can resolve build issues in older ODBC versions.
* ``NANODBC_ENABLE_LIBCXX`` - ``ON|OFF``, default is ``ON``.
  Enables usage of libc++ if found on the system.
* ``NANODBC_EXAMPLES`` - ``ON|OFF``, default is ``ON``.
  Enables building of examples.
* ``NANODBC_HANDLE_NODATA_BUG`` - ``ON|OFF``, default is ``OFF``.
  Provided to resolve issue `#33 <https://github.com/lexicalunit/nanodbc/issues/33>`_.
* ``NANODBC_INSTALL`` - ``ON|OFF``, default is ``ON``.
  Enables install target.
* ``NANODBC_ODBC_VERSION`` - ``SQL_OV_ODBC3`` or ``SQL_OV_ODBC3_80``,
  default is ``SQL_OV_ODBC3_80``, if available.
  Sets the ODBC version macro for nanodbc to use.
* ``NANODBC_TEST`` - ``ON|OFF``, default is ``ON``.
  Enables tests target (alias ``check``).
* ``NANODBC_USE_BOOST_CONVERT`` - ``ON|OFF``, default is ``OFF``.
  Provided as workaround to issue `#44 <https://github.com/lexicalunit/nanodbc/issues/44>`_.
* ``NANODBC_USE_UNICODE`` - ``ON|OFF``, default is ``OFF``.
  Enables full unicode support. ``nanodbc::string`` becomes ``std::u16string`` or ``std::u32string``.

If you are not using CMake to build nanodbc, you will need to set the options,
using the corresponding names, as preprocessor defines yourself.

iODBC and unixODBC
------------------------------------------------------------------------------

Notes about using nanodbc with iODBC and unixODBC in Unix systems.

On Windows, ``sizeof(wchar_t) == sizeof(SQLWCHAR) == 2``.
On Unix, ``sizeof(wchar_t) == 4``.

On unixODBC, ``sizeof(SQLWCHAR) == 2``.
On iODBC, ``sizeof(SQLWCHAR) == sizeof(wchar_t) == 4``.

This leads to incompatible ABIs between applications and drivers.
If building against iODBC and the build option ``NANODBC_USE_UNICODE``
is ``ON``, then ``nanodbc::string_type`` will be ``std::u32string``.

In ALL other cases it will be ``std::u16string``.

The nanodbc continuous integration tests run on `Travis CI`_.
The build platform does not make available a Unicode-enabled iODBC driver.
As such there is no guarantee that tests will pass in entirety on a system using iODBC.
Our recommendation is to use unixODBC.

If you must use iODBC, consider disabling unicode mode to avoid ``wchar_t`` issues.

Test
==============================================================================

*TODO*: How to test your nanodbc build

******************************************************************************
Binaries
******************************************************************************

This section aim to list all known binary packages of nanodbc.

If you maintain binary package of nanodbc and you'd like to list it here,
please submit new entry via pull request or
`open an issue <https://github.com/lexicalunit/nanodbc/issues/new>`_

Windows
==============================================================================

* vcpkg `port of nanodbc <https://github.com/Microsoft/vcpkg/tree/master/ports/nanodbc>`_

.. _`CMake`: https://cmake.org
.. _`Boost.Locale`: https://www.boost.org/doc/libs/release/libs/locale/
.. _`libc++`: https://libcxx.llvm.org
.. _`Travis CI`: https://travis-ci.org/lexicalunit/nanodbc
