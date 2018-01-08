##############################################################################
Develop
##############################################################################

Notes about development process of nanodbc.

******************************************************************************
Contributing
******************************************************************************

nanodbc is an Open Source Software and very accepting of bug fixes
and new features.

Please consider contributing any changes you make via
`pull requests <https://github.com/nanodbc/nanodbc/pulls>`_
or reporting any
`issues <https://github.com/nanodbc/nanodbc/issues>`_ you might have.

Cheers!

******************************************************************************
Guidelines
******************************************************************************

Style
==============================================================================

`clang-format <http://clang.llvm.org/docs/ClangFormat.html>`_
handles all C++ code formatting for nanodbc.

This utility is `brew-installable <https://brew.sh/>`_ on OS X
(``brew install clang-format``) and is available on all major platforms.

See our `.clang-format <https://github.com/nanodbc/nanodbc/blob/master/.clang-format>`_
configuration file for details on the style.

The script `utility/style.sh <https://github.com/nanodbc/nanodbc/blob/master/utility/style.sh>`_
formats all code in the repository automatically.
To run ``clang-format`` on a single file use the following.

.. code-block:: console

  clang-format -i /path/to/file

.. important:: Please auto-format all code submitted in Pull Requests.

`.editorconfig <http://editorconfig.org>`_ file is provided to automatically
tell popular code editors about the preferred basic style settings like
indentation, whitespacesm end of line and such for distinguished types of
plain text files.

******************************************************************************
Environments
******************************************************************************

To get up and running with nanodbc as fast as possible consider
using the provided Dockerfile or Vagrantfile with pre-configured
development environmnets.

Docker
==============================================================================

Spin up a docker container suitable for testing and development of nanodbc:

.. code-block:: console

  $ cd /path/to/nanodbc
  $ docker build -t nanodbc .
  $ docker run -v "$(pwd)":"/opt/$(basename $(pwd))" -it nanodbc /bin/bash
  root@hash:/# mkdir -p /opt/nanodbc/build && cd /opt/nanodbc/build
  root@hash:/opt/nanodbc/build# cmake ..
  root@hash:/opt/nanodbc/build# make nanodbc

Vagrant
==============================================================================

Launch vagrant VM (using VirtualBox provider for example),
``ssh`` into it and build nanodbc:

.. code-block:: console

  $ cd /path/to/nanodbc
  $ vagrant up
  $ vagrant ssh
  vagrant@vagrant-ubuntu-precise-64:~$ git clone https://github.com/nanodbc/nanodbc.git
  vagrant@vagrant-ubuntu-precise-64:~$ mkdir -p nanodbc/build && cd nanodbc/build
  vagrant@vagrant-ubuntu-precise-64:~$ CXX=g++-5 cmake ..
  vagrant@vagrant-ubuntu-precise-64:~$ make nanodbc

******************************************************************************
Test
******************************************************************************

*TODO*: How to test your feature for nanodbc

******************************************************************************
Release
******************************************************************************

*TODO*: How to release and publish source code

Documentation
==============================================================================

*TODO*: How to generate and publish documentation

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
