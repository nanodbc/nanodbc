.. _use:

##############################################################################
Use
##############################################################################

In order to use the nanodbc library, add ``nanodbc/nanodbc.h`` and ``nanodbc/nanodbc.cpp`` source files to your project. On Visual C++, ``nanodbc/variant_row_cached_result.h`` and ``nanodbc/variant_row_cached_result.cpp`` come along with them; they are built only there, as they depend on ``_variant_t``.

Alternatively, you can build the library with CMake as static or shared library and add it to your project as linker input.

Add ``#include <nanodbc/nanodbc.h>`` in source files where you wish to use nanodbc functions and classes.

The entirety of nanodbc can be found within the single ``nanodbc`` namespace.

Strings are ``nanodbc::string``, which is ``std::string`` unless the library was built with ``NANODBC_ENABLE_UNICODE``, and string literals passed to nanodbc are wrapped in ``NANODBC_TEXT``, which selects the matching literal prefix. Writing both makes the code build either way, see :ref:`iODBC and unixODBC <unicode>` below.

******************************************************************************
Quickstart
******************************************************************************

.. code-block:: cpp

  #include <nanodbc/nanodbc.h>

  #include <cstdlib>
  #include <exception>
  #include <iostream>

  int main() try
  {
    auto const connstr = NANODBC_TEXT("..."); // an ODBC connection string to your database
    nanodbc::connection conn(connstr);
    nanodbc::execute(conn, NANODBC_TEXT("create table t (i int)"));
    nanodbc::execute(conn, NANODBC_TEXT("insert into t values (1)"));

    auto result = nanodbc::execute(conn, NANODBC_TEXT("select i from t"));
    while (result.next())
    {
      std::cout << result.get<int>(0) << std::endl;
    }
    return EXIT_SUCCESS;
  }
  catch (std::exception const& e)
  {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
  }

.. _unicode:

******************************************************************************
iODBC and unixODBC
******************************************************************************

Notes about using nanodbc with `iODBC`_ and `unixODBC`_ in Unix systems.

On Windows, ``sizeof(wchar_t) == sizeof(SQLWCHAR) == 2``. On Unix, ``sizeof(wchar_t) == 4``.

On unixODBC, ``sizeof(SQLWCHAR) == 2``. On iODBC, ``sizeof(SQLWCHAR) == sizeof(wchar_t) == 4``.

This leads to incompatible ABIs between applications and drivers. If building against iODBC and the build option ``NANODBC_ENABLE_UNICODE`` is ``ON``, then ``nanodbc::string`` will be ``std::u32string``.

In every other Unicode build it is a 2-byte string: ``std::wstring`` on Visual C++, ``std::u16string`` elsewhere. With ``NANODBC_ENABLE_UNICODE`` left ``OFF``, which is the default, it is plain ``std::string``.

The nanodbc continuous integration tests run with `GitHub Actions`_. The build platform does not make available a Unicode-enabled iODBC driver. As such there is no guarantee that tests will pass in entirety on a system using iODBC. Our recommendation is to use unixODBC.

If you must use iODBC, consider disabling Unicode mode in nanodbc build configuration to avoid ``wchar_t`` issues, see :ref:`build`.

******************************************************************************
Non-ASCII text
******************************************************************************

In a narrow build, which is the default, text crosses between nanodbc and the driver as bytes in whatever the client character set happens to be, and a character that set cannot represent does not survive the trip. On Linux with unixODBC that set is normally UTF-8, which covers everything; on Windows it is the system ANSI codepage, which does not, so a character outside the basic multilingual plane is lost in both directions. Build with ``NANODBC_ENABLE_UNICODE`` set to ``ON`` to exchange UTF-16 with the driver instead.

Statement text is the more fragile of the two, because unlike a bound parameter it reaches the driver with neither a length nor a type. Bind values as parameters rather than writing them into statement text.

******************************************************************************
Batches and multiple result sets
******************************************************************************

A batch of statements returns one result set per statement, in order, and the counts from ``INSERT``, ``UPDATE`` and ``DELETE`` count as result sets of their own. ``execute`` hands back the first of them, so a batch that modifies rows before selecting any arrives positioned on a count, which has no columns and no rows to read. Calling ``next()`` on it raises ``24000 Invalid cursor state`` rather than returning the rows the ``SELECT`` produced.

``result::next_result`` moves to the following result set:

.. code-block:: cpp

  #include <nanodbc/nanodbc.h>

  #include <cstdlib>
  #include <exception>
  #include <iostream>

  int main() try
  {
    nanodbc::connection conn(NANODBC_TEXT("..."));
    auto results = nanodbc::execute(conn, NANODBC_TEXT(
      "declare @t table (id int); "
      "insert into @t values (1), (2); "
      "select id from @t;"));

    results.next_result(); // past the insert's count, onto the select's rows
    while (results.next())
    {
      std::cout << results.get<int>(0) << std::endl;
    }
    return EXIT_SUCCESS;
  }
  catch (std::exception const& e)
  {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
  }

On SQL Server, ``SET NOCOUNT ON`` withholds the counts instead, leaving the rows as the only result set and removing the need to step over anything.

******************************************************************************
Examples
******************************************************************************

The programs under `example`_ are built by the ``examples`` target. Most of them take a connection string as their first argument. ``usage.cpp`` walks through most of the library:

.. literalinclude:: ../example/usage.cpp
   :language: cpp

The rest cover a topic apiece:

* ``northwind.cpp`` queries the Northwind sample database through a data source of that name, so it takes no argument.
* ``rowset_iteration.cpp`` fetches results a rowset at a time.
* ``table_schema.cpp`` reads catalog information, and takes a table name after the connection string, optionally followed by a schema name.
* ``table_valued_parameter.cpp`` binds a SQL Server table-valued parameter.
* ``empty.cpp`` is generated from ``empty.cpp.in`` on the first build and left untracked, as a place to try things out.

``example_unicode_utils.h``, which they share, is where the ``convert`` calls in the listing above come from: it converts between ``nanodbc::string`` and ``std::string`` so that the examples print the same way in either build.

.. _`GitHub Actions`: https://github.com/nanodbc/nanodbc/actions
.. _`example`: https://github.com/nanodbc/nanodbc/tree/main/example
.. _`iODBC`: http://www.iodbc.org
.. _`unixODBC`: http://www.unixodbc.org
