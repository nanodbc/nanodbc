# ChangeLog

## v2.15.1

Added support for C++17 type std::optional with backward compatibility in C++14 via std::experimental::optional.

## v2.15.0

### New Features

- BREAKING: Public API function `complete` has been removed, use `complete_execute` instead [`#314`](https://github.com/nanodbc/nanodbc/pull/314).

### Changes

### Bug Fixes

### Acknowledgements

[Amy Troschinetz](https://github.com/lexicalunit),
[Mateusz Loskot](https://github.com/mloskot),

## v2.14.0

### New Features

- Added Add `find_procedures` and `find_procedure_columns` to `catalog` class [`#249`](https://github.com/nanodbc/nanodbc/pull/249)
- Added support for binding `std::string_view` in `statement` class [`#283`](https://github.com/nanodbc/nanodbc/pull/283)

### Changes

- Changed return type of `result_impl::column_size` from `int` to `long` for consistency [`#261`](https://github.com/nanodbc/nanodbc/pull/261)
- Renamed `VERSION` file to `VERSION.txt` [`#275`](https://github.com/nanodbc/nanodbc/pull/275)

### Bug Fixes

- Fixed SQL statements in `example/usage.cpp` [`#253`](https://github.com/nanodbc/nanodbc/pull/253)
- Fixed `result_impl::column_datatype_name` sizing for Unicode characters [`#263`](https://github.com/nanodbc/nanodbc/pull/263)
- Fixed memory leak of `ensure_pdata` [`#269`](https://github.com/nanodbc/nanodbc/pull/269)
- Fixed retrieval of SQL data type `DATETIMEOFFSET` [`#219`](https://github.com/nanodbc/nanodbc/pull/219)
- Fixed compilation on MacOS using Homebrew's vanilla GCC (for Conan build) [`#279`](https://github.com/nanodbc/nanodbc/pull/279)

### Testing

- Add GitHub Actions with linters [`#273`](https://github.com/nanodbc/nanodbc/pull/273)

### Acknowledgements

Thank you to everyone who contributed to this release by committing changes and submitting pull requests:

[Amy Troschinetz](https://github.com/lexicalunit),
[Bernardo Sulzbach](https://github.com/bernardosulzbach),
[Denis Glazachev](https://github.com/traceon),
[detule](https://github.com/detule),
[Ezequiel Ruiz](https://github.com/emruiz81),
[Joe Siltberg](https://github.com/joesiltberg),
[Mateusz Loskot](https://github.com/mloskot),
[Michael Kaes](https://github.com/mkaes),
[Sewon Park](https://github.com/sphawk)

Thank you to everyone who also opened issues on GitHub.

## v2.13.0

### New Features

- Added support for optional binding to allow out-of-order retrieval of unbound columns with `SQLGetData` [`#236`](https://github.com/nanodbc/nanodbc/pull/236)
- Added `catalog::find_table_privileges` method [`#204`](https://github.com/lexicalunit/nanodbc/pull/204)
- Added `connection::allocate` method to manage ODBC handles handles [`#147`](https://github.com/lexicalunit/nanodbc/pull/147)
- Added `connection::get_info` method which gets string information from a connection [`#215`](https://github.com/lexicalunit/nanodbc/pull/215)
- Added `NANODBC_DEPRECATED` macro [`#279`](https://github.com/lexicalunit/nanodbc/pull/279)
- Added `nanodbc::list_drivers` free function [`#192`](https://github.com/lexicalunit/nanodbc/pull/192)
- Added `nanodbc::list_datasources` free function [`#237`](https://github.com/lexicalunit/nanodbc/pull/237)
- Added `result::column_datatype_name` method [`#237`](https://github.com/lexicalunit/nanodbc/pull/237)
- Added `result::column_decimal_digits` method [`#202`](https://github.com/lexicalunit/nanodbc/pull/202)
- Added `result::has_affected_rows` method [`#185`](https://github.com/lexicalunit/nanodbc/pull/185)
- Added `statement::describe_parameters` method as alternative to `SQLDescribeParam` [`#225`](https://github.com/nanodbc/nanodbc/pull/225)
- Added build flag `NANODBC_DISABLE_ASYNC` which disables async features [`#142`](https://github.com/lexicalunit/nanodbc/pull/142)
- Added CMake package configuration [`#245`](https://github.com/lexicalunit/nanodbc/pull/245)
- Added column validating function to the `result_impl` class [`#206`](https://github.com/lexicalunit/nanodbc/pull/206)
- Added handling of `SQL_SS_UDT` data as binary [`#148`](https://github.com/lexicalunit/nanodbc/pull/148)
- Added input iterator for result class [`#155`](https://github.com/lexicalunit/nanodbc/pull/155)
- Added public macro `NANODBC_THROW_NO_SOURCE_LOCATION` [`#184`](https://github.com/nanodbc/nanodbc/pull/184)
- Added string converter functions for more efficient processing [`#151`](https://github.com/lexicalunit/nanodbc/pull/151)
- Added support for `SQL_WLONGVARCHAR` data type [`#211`](https://github.com/lexicalunit/nanodbc/pull/211)
- Added support for `SQL_SS_XML` data type [`#238`](https://github.com/lexicalunit/nanodbc/pull/238)
- Added support for `std::vector` of strings input [`#214`](https://github.com/lexicalunit/nanodbc/pull/214)
- Added support for `time` column type [`#183`](https://github.com/lexicalunit/nanodbc/pull/183)
- Added support for binding of binary data [`#219`](https://github.com/lexicalunit/nanodbc/pull/219)
- Added support to get binary data as array of bytes [`#130`](https://github.com/lexicalunit/nanodbc/pull/130)
- Added two `catalog` operations: `list_catalogs` and `list_schemas` [`#193`](https://github.com/lexicalunit/nanodbc/pull/193)
- Added very minimal support for SQL Server-specific time datatypes [`#228`](https://github.com/lexicalunit/nanodbc/pull/228)
- Allowed binding values of all intrinsic integral types [`#232`](https://github.com/lexicalunit/nanodbc/pull/232)

### Changes

- Changed `COLUMN_SIZE` for `bytea` to now equal `SQL_NO_TOTAL(-4)` by default [`#251`](https://github.com/lexicalunit/nanodbc/pull/251)
- Disabled declaration of async methods if `NANODBC_DISABLE_ASYNC` is defined [`#197`](https://github.com/lexicalunit/nanodbc/pull/197)
- Fixed, improved and cleaned up the family of bind functions
- Made `NANODBC_TEXT` macro public [`#151`](https://github.com/lexicalunit/nanodbc/pull/151)
- Refactored CMake options to default value `OFF` [`#260`](https://github.com/lexicalunit/nanodbc/pull/260)
- Removed unused output connection string from `SQLDriverConnect` call [`#188`](https://github.com/lexicalunit/nanodbc/pull/188)
- Renamed `nanodbc::string_type` to `nanodbc::string` [`#269`](https://github.com/lexicalunit/nanodbc/pull/269)
- Renamed `src` directory to `nanodbc` [`#256`](https://github.com/lexicalunit/nanodbc/pull/256)
- Replaced custom `NANODBC_STATIC` option with CMake native `BUILD_SHARED_LIBS` [`#250`](https://github.com/lexicalunit/nanodbc/pull/250)
- Report `SQL_HANDLE_DBC` error if statement::open fails to allocate handle [`#178`](https://github.com/lexicalunit/nanodbc/pull/178)
- Started enforcing project-wide consistent code style using `clang-format` [`#203`](https://github.com/lexicalunit/nanodbc/pull/203)

### Bug Fixes

- Added DB-specific tests for `result::affected_rows` [`#154`](https://github.com/lexicalunit/nanodbc/pull/154)
- Fixed `statement_impl::async*` members which were left uninitialized if not built-in [`#187`](https://github.com/lexicalunit/nanodbc/pull/187)
- Fixed binding of `SQL_DECIMAL` and `SQL_NUMERIC` type as character data [`#238`](https://github.com/lexicalunit/nanodbc/pull/238)
- Fixed compilation using Xcode 11 [`#224`](https://github.com/lexicalunit/nanodbc/pull/224)
- Fixed copying of buffer to output string for `SQL_C_BINARY` [`#129`](https://github.com/lexicalunit/nanodbc/pull/129)
- Fixed correct buffer size passed to `SQLGetData` [`#150`](https://github.com/lexicalunit/nanodbc/pull/150)
- Fixed incorrect size passed to `SQLBindParameter` while inserting batch of strings [`#116`](https://github.com/lexicalunit/nanodbc/issues/116)
- Fixed integer conversions [`#176`](https://github.com/lexicalunit/nanodbc/pull/176)
- Fixed issue withSAP/Sybase ASE ODBC driver not setting `sqlsize` to 0 when retrieving `varchar` columns [`#275`](https://github.com/lexicalunit/nanodbc/pull/275)
- Fixed overflowing transaction counter [`#144`](https://github.com/lexicalunit/nanodbc/pull/144)
- Fixed retrieving long strings from MySQL [`#212`](https://github.com/lexicalunit/nanodbc/pull/212)
- Fixed some issues with the async support, plus add async prepare and next [`#170`](https://github.com/lexicalunit/nanodbc/pull/170)
- Fixed to use correct wide-char count when copying from `SQLGetData` buffer [`#182`](https://github.com/lexicalunit/nanodbc/pull/182)
- Handled `SQLGetData` return value of `SQL_NO_TOTAL` [`#161`](https://github.com/lexicalunit/nanodbc/pull/161)
- Put the string lengths in their proper place [`#165`](https://github.com/lexicalunit/nanodbc/pull/165)
- Resolved narrowing from `wchar_t` to `char` warning in VS 2017 updates [`#199`](https://github.com/nanodbc/nanodbc/pull/199)
- Resolved unexpected `bind()` with nulls set to `nullptr` behavior [`#140`](https://github.com/lexicalunit/nanodbc/pull/140)
- Updated to catch up with breaking change in SQLite ODBC 0.9996 [`#165`](https://github.com/nanodbc/nanodbc/pull/165)

### Testing

- Added `integer_boundary` test case for SQLite [`#174`](https://github.com/lexicalunit/nanodbc/pull/174)
- Added AppVeyor build targeting SQL Server 2016 [`#194`](https://github.com/lexicalunit/nanodbc/pull/194)
- Added CI job to lint and build docs [`#152`](https://github.com/lexicalunit/nanodbc/pull/152)
- Added CI job to run clang-format 5.0 to check for code formatting errors [`#153`](https://github.com/lexicalunit/nanodbc/pull/153)
- Added CI jobs to run static code analysis [`#270`](https://github.com/lexicalunit/nanodbc/pull/270)
- Added MinGW build job to AppVeyor [`#196`](https://github.com/lexicalunit/nanodbc/pull/196)
- Added SQL Server test for the Invalid Descriptor Index issue [`#227`](https://github.com/lexicalunit/nanodbc/pull/227)
- Added SQL Server test inserting large blob using direct `INSERT` [`#186`](https://github.com/lexicalunit/nanodbc/pull/186)
- Added test for `std::vector<bool>` workaround [`#267`](https://github.com/lexicalunit/nanodbc/pull/267)
- Added test for integer to string conversion (SQLite only) [`#190`](https://github.com/lexicalunit/nanodbc/pull/190)
- Added test insert and select from/into `nanodbc::time` (SQLite) [`#195`](https://github.com/lexicalunit/nanodbc/pull/195)
- Added tests for PostgreSQL time/timestamp with/without time zone [`#229`](https://github.com/lexicalunit/nanodbc/pull/229)
- Added Vertica to Travis CI [`#199`](https://github.com/lexicalunit/nanodbc/pull/199)
- Refactored test fixture and split into common utilities base and test case base [`#225`](https://github.com/lexicalunit/nanodbc/pull/225)
- Updated Catch to 2.4.2 [`#201`](https://github.com/lexicalunit/nanodbc/pull/201)

### Acknowledgements

Thank you to everyone who contributed pull requests for this release:

[Amy Troschinetz](https://github.com/lexicalunit),
[Billy O'Neal](https://github.com/BillyONeal),
[Christopher Blaesius](https://github.com/ChrisBFX),
[Denis Glazachev](https://github.com/traceon),
[detule](https://github.com/detule),
[Diego Sogari](https://github.com/dsogari),
[H1X4Dev](https://github.com/H1X4Dev),
[Jim Hester](https://github.com/jimhester),
[Jon Valvatne](https://github.com/jon-v),
[Kun Ren](https://github.com/renkun-ken),
[Mateusz Loskot](https://github.com/mloskot),
[Michael C. Grant](https://github.com/mcg1969),
[Rafee Memon](https://github.com/rafeememon),
[Sauron](https://github.com/saur0n),
[Seth Shelnutt](https://github.com/Shelnutt2),
[ThermoX360](https://github.com/ThermoX360),
[whizmo](https://github.com/whizmo)

## v2.12.4

Resolves a possible crash with `SQLDescribeParam()`. In Progress OpenEdge 11 driver setting the
nullableptr argument to null causes a crash. This does not affect SQLite or MySQL drivers.

Thanks to [@AndrewJD79](https://github.com/AndrewJD79) for finding and diagnosing the issue!

## v2.12.3

Unicode: Resolves a major issue with BLOB datatype handling for BINARY and TEXT columns.

## v2.12.2

Resolves a major issue with BLOB datatype handling for BINARY and TEXT columns.

## v2.12.1

Resolves a Travis-CI build issue.

## v2.12.0

Major work undertaken by Mateusz Łoskot provides new features and a host of bug fixes throughout.
Refactoring work moves nanodbc away from platform dependent `wchar_t` in favor of `char16_t` or in the
case of iODBC with Unicode build enabled, `char32_t`. Boost.Test dropped in this version, in favor of Catch.

## New Features

- Converts usages of `wstring` and `wchar_t` to `u16string` and `char16_t`.
- Enable iODBC + Unicode support with `u32string` types.
- Add example program `table_schema.cpp`.
- Add `dbms_name()` and `dbms_version()` methods to `connection` class.

## Testing

- Migrates tests from Boost.Test to Catch framework.
- Enables Unicode tests on Travis CI.
- Syncs `Dockerfile` and `Vagrantfile`; adds quick usage docs for vagrant.
- Switch Dockerfile over to `ubuntu:precise` (default).
- Improve `odbc_test.cpp` to cope with DBMS variations.

## Bug Fixes

- Fix compiler warnings while building with VS2015.
- Add missing optional `schema_name` parameter to usage info.
- Workaround for VS2015 bug in `std::codecvt` for `char16_t`.
- Fix retrieval of variable-length data in parts.
- Fix `catalog::columns::is_nullable()` to handle valid `NULL`.
- Fix check of total of characters required to display `SQL_DATE`.
- Fix `SELECT` result sorting with `NULL` values involved.

## v2.11.3

- Fixes segmentation fault issue with unixODBC on Linux systems.
- Adds support for `while(!results.end())` style iteration.

## v2.11.2

- Adds this CHANGELOG.md file. Future releases should update it accordingly!
- Adds CHANGELOG.md helper script.

## v2.11.1

## New Features

- Major thanks again to Mateusz Łoskot for all the new features!
- Adds convenient access to catalog objects (tables, columns, primary keys).
- Adds `database_name` and `catalog_name` methods to connection class.
- Adds CMake option `NANODBC_ENABLE_LIBCXX` to enable/disable libc++ builds.
- Adds CMake option `NANODBC_EXAMPLES` to enable/disable the example target.
- Adds a `latest` release branch to track most recent release.

## Testing

- Massive updates to Travis CI continuous integration.
- Adds general `odbc_test` to target variety of ODBC drivers.
- Adds specific MySQL tests.
- Updates test target organization.
  - The way the targets were designed is such that:
    - test: runs all tests, but will not build them
    - tests: builds all tests, but does not run them
    - check: builds all tests and then runs all tests
  - For individual tests then, it makes sense to use:
    - ${name}_test: runs ${name}_test, but will not build it
    - ${name}_tests: builds ${name}_test, but does not run it
    - ${name}_check: builds ${name}_test and then runs it

## Bug Fixes

- Fix test check of `result::affected_rows` for `SELECT` statement.
- Fix `result::position` to make it consistent with `SQL_ATTR_ROW_NUMBER`.
- Fix string object construction syntax.
- Adds missing `#include <cstring>`.

## Other Changes

- More robust and friendly publish and release scripts.
- Updates to README and documentation.
- Adds `-DUNICODE` and `-D_UNICODE` for Visual Studio projects.
- Adds examples based on the documentation.
- Adds `rowset_iteration` example.

## v2.10.0

## New Features

- Major thanks to Mateusz Łoskot for all the effort!
- Adds Dockerfile to support testing and development.
- Adds build.bat convenience script for Visual Studio users.
- Adds CMake options `NANODBC_INSTALL` and `NANODBC_TEST` to control generation of those targets.

## Bug Fixes

- Fixes cmake build on OS X El Capitan.
- Refine assert in `result_impl::position` with `SQL_ROW_NUMBER_UNKNOWN`.
- MSBuild Platform property for 32-bit is Win32.
- Reset null indicator before move for all columns, not just bound columns.
- Fixes Doxygen generation of macro docs.

## v2.9.1

## New Features

- Adds `Vagrantfile` to support testing and development.
- Adds customizable `NANODBC_ASSERT` macro.
- Adds CMake option `NANODBC_STATIC` (default OFF).
- Clean up Visual C++ 64-bit warnings.

## Bug Fixes

- CMake: Fixes ODBC linking on Unix.
- Adds documentation on is_null() limitation.
- Write null indicator to `cbdata_` if indicated by `SQLGetData`.

## Testing

- Initial configuration of Travis CI build matrix.

## Other Changes

- Added a Contributing section to readme.
- Updates to SQLite tests.
- Disable MSVC warning C4244 in tests.

## v2.8.1

- Update CMakeLists.txt to enable builds with Visual Studio. Thanks Mateusz Łoskot!
- Add async connection support, plus extended database_error info. Thanks Yao Wei Tjong!
- Add linking against ODBC libraries on Windows.
- Change `param_type_from_direction` to throw `programming_error`.
- Define `NANODBC_SNPRINTF` in terms of `_snprintf_s` for MSVC.
- Setting CMake `-DNANODBC_ODBC_VERSION` option now works.

## v2.7.0

- Adds move constructors.
- Fixes Xcode MARK comments.
- Adds section comment banners to header file.
- Removes `throw()` from header files, uses `noexcept` instead.
- Adds basic and SQLite `std::move` test case.

## v2.6.0

- Resolves issue with decimal digits/scale and rounding. Thanks dedomilo!
- Resolve issue with `DECIMAL` to string conversion. Thanks dedomilo!

## v2.5.1

- Disable default Unicode on windows.
- Override ODBC version with `NANODBC_ODBC_VERSION`.

## v2.4.0

- Add `statement::async_execute_direct` and `statement::async_complete`. Thanks Jon Valvatne!
- Add NOEXCEPT define to allow compilation under Visual Studio 2013.

## v2.3.0

- Provides optional Boost workaround for missing `codecvt` support in libstdc++.

## v2.2.3

- Adds minimap banners for code navigation.
- Adds `column_c_datatype()`.
- Converts line endings to Unix.
- Adds `just_execute` class of functions that don't create result objects.

## v2.1.0

- Adds publish script.
- Fixes broken links in readme.
- Use C++11's `=delete` where appropriate.

## v2.0.1

- Fixes many documentation issues.
- Adds more ToDo info about updating docs.
- Adds notes about different versions.
- Cleans up style; removes CPP11 macros and C++03 support cruft.
- Silence warnings and untabify.
- Works with Unicode (`std::wstring` as `nanodbc::string_type`)
- Using nanodbc with SQL Server Native Client works with `nvarchar(max)` and `varchar(max)` fields in Win32 and Win64.

## v1.0.0

Version 1.0.0 and all commits prior are now completely unsupported.
