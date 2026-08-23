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
Dates and times as strings
******************************************************************************

A string bound to a date, time or timestamp parameter is read by the driver, which accepts the literals ODBC spells out and little else: ``yyyy-mm-dd``, ``hh:mm:ss`` and ``yyyy-mm-dd hh:mm:ss[.f...]``, a space between date and time and no time zone offset. That form is the portable one and every driver here stores it correctly.

ISO 8601 looks close enough to pass and is not. Given ``2020-09-03T15:27:38-02:00``, the PostgreSQL driver stores ``2020-09-03 00:00:00`` and reports success, the ``T`` having stopped it reading the time, with nothing in the return code or the diagnostics to say so. SQL Server refuses the same value outright. Writing it as ``2020-09-03 15:27:38`` stores the time on both.

Binding a ``nanodbc::timestamp`` avoids the question, since it carries its fields rather than a spelling of them.

******************************************************************************
Batch parameters and how far they carry
******************************************************************************

Binding arrays and executing them as a batch sets ``SQL_ATTR_PARAMSET_SIZE``, and what happens next is the driver's to decide. A driver that implements array binding sends the sets together; one that does not is free to walk them, executing the statement once per set, and the ODBC API gives the caller no way to tell which it got.

The difference is large. Inserting 5000 rows of two columns, over a local network, at several batch sizes:

===============  ===================  ===================
Batch size       PostgreSQL (rows/s)  SQL Server (rows/s)
===============  ===================  ===================
1                4,322                858
10               21,887               7,480
100              29,043               52,731
1000             32,328               123,524
5000             33,051               140,003
===============  ===================  ===================

SQL Server's driver keeps gaining as the batch grows. The PostgreSQL driver stops gaining after a hundred or so, a batch fifty times larger buying another tenth, which is the shape of a driver walking the sets rather than sending them.

Where a driver walks them, a single statement carrying many rows is faster than many parameter sets — around thirteen times, in the same measurement. It costs the safety of bound parameters, so build it from values you trust or bind a smaller batch and accept the rate.

None of this applies to a database in the same process. SQLite has no round trip to save and shows no difference between the two.

******************************************************************************
Binding a batch held as rows
******************************************************************************

Parameters are bound a column at a time, an array to each marker, which is the shape the drivers take. Data usually arrives as rows instead, a struct to each, and turning one into the other means a vector per column and a loop to fill them.

``bind_rows`` does that. Each accessor names one parameter, in the order the markers appear, either as a pointer to a member or as anything callable with a row:

.. code-block:: cpp

    #include <nanodbc/nanodbc.h>
    #include <vector>

    struct person
    {
        long id;
        nanodbc::string name;
    };

    int main()
    {
        nanodbc::connection conn(NANODBC_TEXT("dsn"));
        std::vector<person> people{{1, NANODBC_TEXT("Ada")}, {2, NANODBC_TEXT("Grace")}};

        nanodbc::statement stmt(conn);
        prepare(stmt, NANODBC_TEXT("insert into people (id, name) values (?, ?)"));
        bind_rows(stmt, people, &person::id, &person::name);
        execute(stmt, people.size());
    }

The values are copied into the statement, so the rows are free to go out of scope before it runs. The overloads of ``bind`` taking a pointer bind the caller's buffer instead, which has to stay alive and unchanged until execution.

Built as C++17 or later, an accessor yielding ``std::optional`` binds an absent value as null, which is how a nullable column is filled from a row that has no value for it.

What reaches the driver is a parameter array either way, so this is the same batch described above, and the same limits apply to how far it carries.

******************************************************************************
Reading a result other than forwards
******************************************************************************

``next()`` works on any result. ``first()``, ``last()``, ``prior()``, ``move()`` and ``skip()`` ask the driver to fetch somewhere other than the next row, and a cursor has to be able to scroll for that. ODBC does not give one by default, so the calls fail:

.. code-block:: text

    HY106: [Microsoft][ODBC Driver 18 for SQL Server]Fetch type out of range

``position()`` reporting zero comes from the same place.

The cursor type is a statement attribute, set before the statement runs:

.. code-block:: cpp

    #include <nanodbc/nanodbc.h>
    #include <cstdint>
    #include <list>
    #include <sql.h>
    #include <sqlext.h>

    int main()
    {
        nanodbc::connection conn(NANODBC_TEXT("dsn"));

        std::list<nanodbc::statement::attribute> attributes;
        attributes.push_back({SQL_ATTR_CURSOR_TYPE, 0, (std::uintptr_t)SQL_CURSOR_STATIC});

        nanodbc::statement stmt(conn, attributes);
        prepare(stmt, NANODBC_TEXT("select i from t order by i asc"));
        auto results = execute(stmt);

        results.last();
        results.prior();
        results.first();
        results.move(2);  // absolute, counted from one
    }

``SQL_CURSOR_STATIC`` takes a snapshot of the rows and scrolls over it. ``SQL_CURSOR_DYNAMIC`` and ``SQL_CURSOR_KEYSET_DRIVEN`` scroll as well and see later changes to differing degrees, at a cost the driver decides. A scrolling cursor is dearer than the forward only one, which is why it is not what you get without asking.

SQL Server, PostgreSQL, MySQL, SQLite and Oracle all honour ``SQL_CURSOR_STATIC`` through their ODBC drivers. A driver that cannot give the cursor asked for is free to substitute another and say so through ``SQLGetDiagRec`` rather than to fail, so a result that still will not scroll is worth checking the diagnostics for.

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
