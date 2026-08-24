.. _index:

##############################################################################
nanodbc - C++ wrapper for ODBC API
##############################################################################

nanodbc is a small library that makes ODBC API programming easy and fun again.

This documentation is for version |release_tag|. Earlier versions are listed under :ref:`Older Versions <versions>`.

******************************************************************************
Motivation
******************************************************************************

Here's a before and after look at straight ODBC C API code and its equivalent nanodbc code.

Reading three columns of every row, through the ODBC C API:

.. code-block:: cpp
   :class: comparison

    #include <sql.h>
    #include <sqlext.h>
    #include <stdio.h>

    #define NAME_LEN 50
    #define PHONE_LEN 20

    int main()
    {
        SQLHENV henv;
        SQLHDBC hdbc;
        SQLHSTMT hstmt = 0;
        SQLRETURN retcode;
        SQLCHAR sCustID[NAME_LEN], szName[NAME_LEN], szPhone[PHONE_LEN];
        SQLLEN cbName = 0, cbCustID = 0, cbPhone = 0;

        retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            retcode =
                SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
            {
                retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                {
                    SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
                    retcode = SQLConnect(
                        hdbc, (SQLCHAR*)"NorthWind", SQL_NTS, NULL, 0, NULL, 0);
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                    {
                        retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
                        retcode = SQLExecDirect(
                            hstmt,
                            (SQLCHAR*)"SELECT CustomerID, ContactName, Phone"
                                      " FROM CUSTOMERS ORDER BY 2, 1, 3",
                            SQL_NTS);
                        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                        {
                            SQLBindCol(
                                hstmt, 1, SQL_C_CHAR, sCustID, NAME_LEN, &cbCustID);
                            SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, NAME_LEN, &cbName);
                            SQLBindCol(
                                hstmt, 3, SQL_C_CHAR, szPhone, PHONE_LEN, &cbPhone);
                            for (int i = 0;; i++)
                            {
                                retcode = SQLFetch(hstmt);
                                if (retcode == SQL_ERROR ||
                                    retcode == SQL_SUCCESS_WITH_INFO)
                                    printf("error\n");
                                if (retcode == SQL_SUCCESS ||
                                    retcode == SQL_SUCCESS_WITH_INFO)
                                    printf(
                                        "%d: %s %s %s\n",
                                        i + 1,
                                        sCustID,
                                        szName,
                                        szPhone);
                                else
                                    break;
                            }
                            SQLCancel(hstmt);
                            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                        }
                        SQLDisconnect(hdbc);
                    }
                    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
                }
            }
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
        }
    }

The same thing, through nanodbc:

.. code-block:: cpp
   :class: comparison

    #include <nanodbc/nanodbc.h>

    #include <iostream>

    int main()
    {
        nanodbc::connection conn(NANODBC_TEXT("NorthWind"));
        auto rows = nanodbc::execute(
            conn, NANODBC_TEXT("SELECT CustomerID, ContactName, Phone"
                               " FROM CUSTOMERS ORDER BY 2, 1, 3"));

        for (auto& row : rows)
        {
            std::cout << row.get<std::string>(0) << " "
                      << row.get<std::string>(1) << " "
                      << row.get<std::string>(2) << '\n';
        }
    }

The native C API for working with ODBC is exorbitantly verbose, ridiculously complicated, and fantastically brittle. nanodbc addresses these frustrations! The goal for nanodbc is to make developers happy.

The nanodbc philosophy states: common database programming tasks should be easy, requiring concise and simple code.

The latest `C++ standard`_ and `C++ best practices`_ are enthusiastically incorporated to make the library as future-proof as possible.

To accommodate users who can not use the latest and greatest, `semantic versioning`_ and release notes will clarify required C++ features and/or standards for particular versions.

******************************************************************************
History
******************************************************************************

Originally nanodbc began as a simple fork of `TinyODBC`_ with additional features. Eventually it grew to the point where it made sense to break with many of the basic underlaying design decisions of TinyODBC and completely refactor much of the codebase. Other projects that have had influence on nanodbc include `SimpleDB`_, `pyodbc`_, `Database Template Library`_, and `GSODBC`_.

******************************************************************************
Features
******************************************************************************

Why should you use nanodbc?

* Small! Same as TinyODBC, nanodbc is small: a header and an implementation file.
* Simple! There are only a handful of significant classes to learn.
* Portable! nanodbc uses only standard C++ headers in addition to the ODBC API headers.
* Robust! Where it makes sense, error handling is done with exceptions instead of return codes.
* Features! nanodbc supports ODBC 3, SQLDriverConnect(), transactions, bound parameters, bulk operations, and much more.
* Tested! The suites cover SQLite, PostgreSQL, MySQL, MariaDB, SQL Server and Vertica, each of which runs as a container so that none of them has to be installed to work on nanodbc.
* Documented! These pages cover :ref:`installation <install>` and :ref:`usage <use>`, and the :ref:`API reference <api>` is generated from the source.
* Active! nanodbc is maintained and open to contributions, see :ref:`Develop <develop>`.

******************************************************************************
Design
******************************************************************************

All complex objects in nanodbc follow the `pimpl`_ (Pointer to IMPLementation) idiom to provide separation between interface and implementation, value semantics, and a clean nanodbc.h header file that includes nothing but standard C++ headers.

nanodbc wraps ODBC code, providing a simpler way to do the same thing. We try to be as featureful as possible, but I can't guarantee you'll never have to write supporting ODBC code. Personally, I have never had to do so.

Major features beyond what's already supported by ODBC are not within the scope of nanodbc. This is where the nano part of nanodbc becomes relevant: This library is as minimal as possible. That means no dependencies beyond standard C++ and typical ODBC headers and libraries to link against. No features unsupported by existing ODBC API calls.


.. toctree::
  :hidden:

  install
  use
  develop
  api
  versions

.. _`TinyODBC`: https://code.google.com/archive/p/tiodbc/
.. _`SimpleDB`: http://simpledb.sourceforge.net
.. _`PyODBC`: https://github.com/mkleehammer/pyodbc
.. _`Database Template Library`: http://dtemplatelib.sourceforge.net
.. _`GSODBC`: http://www.codeguru.com/cpp/data/mfc_database/odbc/article.php/c4337/A-Simple-and-Smart-ODBC-Wrapper-Library.htm
.. _`C++ standard`: https://isocpp.org/std/status
.. _`C++ best practices`: https://github.com/isocpp/CppCoreGuidelines
.. _`semantic versioning`: http://semver.org
.. _`pimpl`: http://wiki.c2.com/?PimplIdiom
