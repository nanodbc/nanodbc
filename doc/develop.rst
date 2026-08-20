.. _develop:

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

Keeping nanodbc covered with tests is one of the important objectives, so new contributions submitted via pull requests must include corresponding tests.

The tests are built with the ``tests`` target and run with `ctest`_. They are laid out as:

* ``test/base_test_fixture.h`` provides the helpers the fixtures share, such as connecting, creating and dropping tables, and reporting which backend is under test.
* ``test/test_case_fixture.h`` holds the test cases that run against every backend.
* ``test/<database>_test.cpp`` is the source for an independent test program that includes both the common and the database-specific test cases. ``test/main.cpp`` supplies the ``main()`` each of them links against, and ``test/CMakeLists.txt`` lists the databases a program is built for.

To add a new test case, add a method to ``test_case_fixture`` in ``test/test_case_fixture.h``, then copy the ``TEST_CASE_METHOD`` boilerplate into each ``test/<database>_test.cpp``, updating name and tags. A test that only makes sense for one database goes straight into that database's source file instead.

The SQLite and utility tests need no server, so they are the quickest way to check a change. The Vertica tests need Vertica's own ODBC driver, which the development image does not carry, so a full run excludes them:

.. code-block:: console

  $ ctest --test-dir build --output-on-failure -E vertica_tests
  $ ctest --test-dir build --output-on-failure -R sqlite_tests

See `README.md`_ for the rest, under Tests.

******************************************************************************
Release
******************************************************************************

``utility/publish.sh`` bumps the version in ``VERSION.txt``, commits it and pushes the matching ``vX.Y.Z`` tag; pushing the tag is what drives the release and the documentation deployment. Update ``CHANGELOG.md`` first, since the section for the version being released becomes the release notes and the publish script checks that it is there.

See `README.md`_ for the whole process, under Publish and Release Process.

Documentation
==============================================================================

See `doc/README.md`_.

.. _`ctest`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
.. _`README.md`: https://github.com/nanodbc/nanodbc/blob/main/README.md
.. _`doc/README.md`: https://github.com/nanodbc/nanodbc/blob/main/doc/README.md
