##############################################################################
Develop
##############################################################################

Notes about development process of nanodbc.

******************************************************************************
Contributing
******************************************************************************

nanodbc is an Open Source Software and very accepting of bug fixes and new features.

Please consider contributing any changes you make via `pull requests <https://github.com/nanodbc/nanodbc/pulls>`_ or reporting any `issues <https://github.com/nanodbc/nanodbc/issues>`_ you might have.

Cheers!

******************************************************************************
Guidelines
******************************************************************************

Style
==============================================================================

`clang-format <http://clang.llvm.org/docs/ClangFormat.html>`_ handles all C++ code formatting for nanodbc.

See our `.clang-format <https://github.com/nanodbc/nanodbc/blob/main/.clang-format>`_ configuration file for the style, and for the major version of ``clang-format`` it is written against, which is named in the comment at the top of the file. A different major version will reformat code that is already correct, so match the one named there. The development container carries it, and ``pip install clang-format==<major>.*`` supplies it on a host.

To run ``clang-format`` against the whole nanodbc codebase:

.. code-block:: console

  clang-format -i $(git ls-files '*.h' '*.cpp')

`.clang-format-ignore <https://github.com/nanodbc/nanodbc/blob/main/.clang-format-ignore>`_ lists what to leave alone, so vendored code is skipped even when it is named on the command line.

To run ``clang-format`` on a single file use the following.

.. code-block:: console

  clang-format -i /path/to/file

.. important:: Please auto-format all code submitted in Pull Requests.

`.editorconfig <http://editorconfig.org>`_ file is provided to automatically tell popular code editors about the preferred basic style settings like indentation, whitespace, end of line and such for distinguished types of plain text files.

******************************************************************************
Environments
******************************************************************************

To get up and running with nanodbc as fast as possible, use the containers the repository provides. Every database nanodbc is tested against runs as a container, so none of them has to be installed on your machine. `README.md`_ covers the whole setup under Quick Setup for Testing or Development Environments.

Docker
==============================================================================

``docker compose`` brings up the database servers and a development container that carries the compiler toolchain, CMake, and the ODBC driver managers, drivers and client tools for all of them:

.. code-block:: console

  $ cd /path/to/nanodbc
  $ docker compose up -d
  $ docker compose run --rm nanodbc /bin/bash
  root@hash:/opt/nanodbc# cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  root@hash:/opt/nanodbc# cmake --build build --parallel
  root@hash:/opt/nanodbc# ctest --test-dir build --output-on-failure -E vertica_tests

The development container presets the ``NANODBC_TEST_CONNSTR_*`` variables the test programs read, so a build and a test run inside it need no arguments.

To build the development image on its own, without the database services, pass the file to ``docker build`` with ``-f``, since it is not named ``Dockerfile``:

.. code-block:: console

  $ docker build -f Dockerfile.dev -t nanodbc .
  $ docker run -v "$(pwd)":/opt/nanodbc -it nanodbc /bin/bash

******************************************************************************
Test
******************************************************************************

See `README.md`_.

******************************************************************************
Release
******************************************************************************

See `README.md`_.

Documentation
==============================================================================

See `doc/README.md`_.

******************************************************************************
Future
******************************************************************************

Good to Have / Want Someday

* Refactor tests to follow BDD pattern.
* Update codebase to use more C++14 idioms and patterns.
* Write more tests with the goal to have much higher code coverage.
* More tests for a large variety of drivers. Include performance tests.
* Clean up ``bind_*`` family of functions, reduce any duplication.
* Improve documentation: The main website and API docs should be more responsive.
* Provide more examples in documentation, more details, and point out any gotchas.
* Versioned generated source level API documentation for release and latest. For each major and minor published versions too?
* Add "HOWTO Build" documentation for Windows, OS X, and Linux.

.. _`README.md`: https://github.com/nanodbc/nanodbc/blob/main/README.md
.. _`doc/README.md`: https://github.com/nanodbc/nanodbc/blob/main/doc/README.md
