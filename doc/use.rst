.. _use:

##############################################################################
Use
##############################################################################

In order to use the nanodbc library, add ``nanodbc/nanodbc.h``
and ``nanodbc/nanodbc.cpp`` source files to your project.

Alternatively, you can build the library with CMake as static or shared
library and add it to your project as linker input.

Add ``#include <nanodbc/nanodbc.h>`` in source files where you wish to use
nanodbc functions and classes.

The entirety of nanodbc can be found within the single `nanodbc` namespace.

******************************************************************************
Quickstart
******************************************************************************

.. code-block:: cpp

  #include <nanodbc/nanodbc.h>
  #include <exception>

  int main() try
  {
    auto const connstr = ...; // an ODBC connection string to your database
    nanodbc::connecton conn(connstr);
    nanodbc::execute(conn, NANODBC_TEXT("create table t (i int)"));
    nanodbc::execute(conn, NANODBC_TEXT("insert into t (1)"));

    auto result = nanodbc::execute(conn, NANODBC_TEXT("SELECT i FROM t"));
    while (result.next())
    {
      auto i = result.get<int>(0);
    }
    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
  {
      cerr << e.what() << endl;
      return EXIT_FAILURE;
  }


******************************************************************************
iODBC and unixODBC
******************************************************************************

Notes about using nanodbc with `iODBC`_ and `unixODBC`_ in Unix systems.

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

If you must use iODBC, consider disabling Unicode mode in nanodbc build
configuration to avoid ``wchar_t`` issues, see :ref:`build`.

.. _`iODBC`: http://www.iodbc.org
.. _`unixODBC`: http://www.unixodbc.org
.. _`Travis CI`: https://travis-ci.org/lexicalunit/nanodbc
