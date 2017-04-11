##############################################################################
Use
##############################################################################

Notes about programming aspects of using nanodbc in client applications. 

******************************************************************************
Quickstart
******************************************************************************

.. code-block:: cpp

  #include <nanodbc/nanodbc.h>
  #include <exception>

  int main() try
  {
    auto const connstr = ...;
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
