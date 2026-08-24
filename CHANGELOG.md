# ChangeLog

## v3.0.0

- The documentation's landing page is titled "nanodbc - C++ ODBC wrapper", which fits on one line where the longer wording wrapped, the heading's own anchor link taking up the width that tipped it over.
- nanodbc requires C++17. C++11 and C++14 are no longer supported, and the conditionals standing in for `std::optional`, `std::variant`, `std::any` and `std::string_view` are gone, the four being there unconditionally. The `NANODBC_HAS_STD_*` macros are still defined, so code asking whether the feature is there keeps compiling.
- `statement::bind` and `statement::bind_strings` take a `std::optional`, and a vector of them, so an absent value binds as null without a sentry value or an array of flags kept in step with the values. The sentry and flag overloads remain.
- The observers whose answer is the whole point are `[[nodiscard]]`: `result::next` and the rest of the navigation, `is_null`, `get`, `get_as` and the column accessors, along with `connected`, `rows`, `columns` and `affected_rows`. Discarding one of them was a quiet way to read the wrong row. `execute` and `transact` are left alone, ignoring their result being a fair thing to do.
- The `attribute` class that stood in for `std::variant` below C++17 is gone, as are the `#else` halves of `statement::attribute` and `connection::attribute` that went with it.
- CI builds against C++17 and C++20 and the suites run at both; the C++14 jobs are gone. The MinGW job passes the standard its matrix names, having declared one and then configured without it.
- Internals follow the standard they are now written against: fold expressions where a braced list was standing in for one, `if constexpr` where a pair of `enable_if` overloads differed only by a compile-time condition, `std::optional` read through one trait rather than two, and `using` rather than `typedef`.
- The documentation build is warning-free again: `PARAM_RETURN`'s comment gave Doxygen a code span it read as two sentences, `JAVADOC_AUTOBRIEF` ending the brief at the `?` inside it, and `doc/README.md` still described the Breathe step that generating the reference with Doxygen replaced.
- The before and after comparison on the landing page is code rather than two screenshots, so it is highlighted by the same theme as the rest of the site, selectable, searchable and legible at any width. Both halves compile, which the lint run checks: the ODBC one against the ODBC headers rather than Windows-only ones, and the nanodbc one in a narrow build and a Unicode one alike, reading its rows through a range-for rather than counting them.
- The documentation site and the API reference beside it read as one site. The pages are themed with Furo, whose colours, fonts and sizes `conf.py` sets from doxygen-awesome-css's own custom properties, and the reference follows the reader's system between light and dark the way the pages do, which being pinned light it did not.
- Describing a parameter and finding a bound column look the key up once rather than twice, `count()` followed by `at()` having asked the same map the same question either side of the answer.

## v2.20.0

- `connection::browse_connect` asks the driver what it needs in order to connect, which SQL Server's answers with the attributes it wants and most others decline. [`#235`](https://github.com/nanodbc/nanodbc/issues/235)
- `result::get_as` reads a column as the type the driver says it is, into a `std::variant` naming the alternatives to choose between. [`#324`](https://github.com/nanodbc/nanodbc/issues/324)
- A parameter described with `describe_parameters` can be bound without preparing first, which is what a stored procedure with known parameters wants. [`#207`](https://github.com/nanodbc/nanodbc/issues/207)
- `cmake-lint` reads the CMake scripts in CI; `cmake-format` is not run, its output rewriting them whole. [`#394`](https://github.com/nanodbc/nanodbc/issues/394)
- PostgreSQL is tested through its Unicode driver, the ANSI one being unable to take the wide parameter a Unicode build binds. [`#519`](https://github.com/nanodbc/nanodbc/issues/519)
- `result::get` reads a column into a `std::any`, holding what the column holds, or nothing where it is null. [`#325`](https://github.com/nanodbc/nanodbc/issues/325)
- Every diagnostic a failure carries reaches the message, where all but the first were dropped outside Windows, and each arrives once rather than trailing the buffer it was read into. [`#315`](https://github.com/nanodbc/nanodbc/issues/315)
- `result::get_ref` builds the value before reading a column into a `std::optional`, having read into one that held nothing, which crashes for a wide string. [`#525`](https://github.com/nanodbc/nanodbc/issues/525)
- `statement::parameter_is_null` says whether a parameter bound for output came back null, which its value cannot, the driver leaving the caller's buffer alone. [`#436`](https://github.com/nanodbc/nanodbc/issues/436)
- The suites run at C++17 as well as C++14, so what is written behind `std::optional` and `std::variant` is executed rather than only compiled. [`#514`](https://github.com/nanodbc/nanodbc/issues/514)
- A test covers a connection to each of several threads, which is the arrangement the library asks for and nothing exercised. [`#285`](https://github.com/nanodbc/nanodbc/issues/285)

## v2.19.0

- A date or time parameter bound as text is declared as the driver describes it, Oracle refusing the ODBC literal form when it is declared as text. [`#248`](https://github.com/nanodbc/nanodbc/issues/248)
- `statement::execute` takes a `batch_ops`, so the rowset size and the parameter array length can differ. [`#162`](https://github.com/nanodbc/nanodbc/issues/162)
- Documented that how far a batch of parameters carries is the driver's to decide, with measurements. [`#241`](https://github.com/nanodbc/nanodbc/issues/241)
- Oracle runs in the test suite and in CI, against Oracle Database Free through Instant Client, carrying the cases the other backends share. [`#487`](https://github.com/nanodbc/nanodbc/issues/487)
- The Oracle job starts a prepared database rather than opening one on first boot, which is most of the difference between three minutes fifty and two minutes five.
- `bind_rows` binds a batch held as rows, naming one accessor per parameter, and copies the columns into the statement so the rows need not outlive it. [`#443`](https://github.com/nanodbc/nanodbc/issues/443)
- The constructors taking connection and statement attributes are reachable at C++14, the nested `attribute` having been private there and the `connection` overloads compiled out altogether. [`#494`](https://github.com/nanodbc/nanodbc/issues/494)
- Connection attributes reach the driver before it connects, which is what several of them govern; setting them on an allocated handle beforehand cannot work, since `connect()` allocates another. [`#215`](https://github.com/nanodbc/nanodbc/issues/215)
- A test covers reading a result other than forwards, which asks the driver for a cursor that scrolls; `move()` counts rows from one, which it did not say. [`#368`](https://github.com/nanodbc/nanodbc/issues/368)
- Tests pin down what `affected_rows` reports for the statements whose count is defined, a statement matching nothing included. [`#168`](https://github.com/nanodbc/nanodbc/issues/168)
- The SQLite and SQL Server suites also run built for Unicode, and SQL Server is tested through the ODBC driver the development container has used for some time. [`#265`](https://github.com/nanodbc/nanodbc/issues/265)
- `test_std_optional` leaves out its date and time cases on Oracle, which has no TIME type and describes a DATE as a timestamp, as the tests they wrap are already left out there.
- Documented reading many rows into containers, which is the rowset size and `get_ref` rather than any binding of output buffers. [`#163`](https://github.com/nanodbc/nanodbc/issues/163)

## v2.18.0

- The API reference is a Doxygen site themed with doxygen-awesome-css, with search and a navigation tree. [`#373`](https://github.com/nanodbc/nanodbc/issues/373)
- Five functions returning nothing documented a return value, which Doxygen reported and Breathe did not.
- A test covers a batch delete whose first parameter set matches nothing, which the drivers count differently and carry out alike. [`#168`](https://github.com/nanodbc/nanodbc/issues/168)
- A test covers the transaction isolation level set by a statement holding for the session, as it is the session's to hold. [`#10`](https://github.com/nanodbc/nanodbc/issues/10)
- A floating point column read as text carries every digit needed to read back as the same value, where six decimals were rendered whatever the magnitude. [`#196`](https://github.com/nanodbc/nanodbc/issues/196)
- A test covers binding a timestamp's fractional second, which counts nanoseconds and has to suit what the column resolves. [`#4`](https://github.com/nanodbc/nanodbc/issues/4)
- A date or time parameter bound as text is declared as text, so the server reads it rather than the driver, which reads fewer spellings. [`#248`](https://github.com/nanodbc/nanodbc/issues/248)
- A test covers reading a generated identity back from an INSERT through the OUTPUT clause. [`#193`](https://github.com/nanodbc/nanodbc/issues/193)
- Documented that a batch returns a result set per statement, counts included, and how to reach the rows. [`#247`](https://github.com/nanodbc/nanodbc/issues/247)
- A test covers binding a string as an output parameter, which the driver writes back into the caller's buffer. [`#231`](https://github.com/nanodbc/nanodbc/issues/231)
- A test reads long text at the sizes either side of the chunk the driver is asked for, where a piece could be dropped or repeated. [`#22`](https://github.com/nanodbc/nanodbc/issues/22)
- `transaction::commit()` honours the connection's rollback flag, so an inner rollback is no longer discarded by an outer commit. [`#78`](https://github.com/nanodbc/nanodbc/issues/78)
- A regression test covers binding objects past the driver's reported parameter limit, which nothing pinned. [`#5`](https://github.com/nanodbc/nanodbc/issues/5)
- A bound character column the driver under-sized is read again in full instead of coming back truncated. [`#343`](https://github.com/nanodbc/nanodbc/issues/343)
- Tests cover executing a prepared statement repeatedly and binding a batch with an arithmetic null sentry. [`#56`](https://github.com/nanodbc/nanodbc/issues/56) [`#77`](https://github.com/nanodbc/nanodbc/issues/77)
- Column buffer casts go through `void*` rather than `reinterpret_cast` and a C-style cast, which analysers report as unsafe. [`#420`](https://github.com/nanodbc/nanodbc/issues/420)
- A null timestamp read after `unbind()` yields the fallback or raises `null_access_error`, where it once aborted. [`#423`](https://github.com/nanodbc/nanodbc/issues/423)
- `PARAM_RETURN` documents that it binds a procedure's return value at parameter zero of `{ ? = CALL proc(?) }`. [`#355`](https://github.com/nanodbc/nanodbc/issues/355)
- Documented that non-ASCII text belongs in bound parameters, not statement text, whose encoding a narrow build leaves to the driver. [`#359`](https://github.com/nanodbc/nanodbc/issues/359)

## v2.17.0

- A batch of one can bind NULL. The null indicator was withheld from the driver whenever the batch held a single value. [`#220`](https://github.com/nanodbc/nanodbc/issues/220) [`#347`](https://github.com/nanodbc/nanodbc/issues/347)
- The last two `__GNUC__` conditions are gone; both were unreachable at C++14, which is the minimum. [`#383`](https://github.com/nanodbc/nanodbc/issues/383)
- Every integer width renders as a string, where only 32 bits and wider did. [`#467`](https://github.com/nanodbc/nanodbc/issues/467)
- The tests say `SQLWCHAR` rather than `WCHAR`, which is a Windows type and left the suite unbuildable where the ODBC headers do not supply it. [`#439`](https://github.com/nanodbc/nanodbc/issues/439)
- `SQLBindParameter` is given the bound type's size as the buffer length rather than the parameter's column size, which is a precision and not a byte count. [`#437`](https://github.com/nanodbc/nanodbc/issues/437)
- A bound C string is given its own length as the buffer length, rather than the parameter's column size, which counts digits. [`#462`](https://github.com/nanodbc/nanodbc/issues/462)
- `result::get(column_name, fallback)` no longer aborts on a null unbound column. [`#480`](https://github.com/nanodbc/nanodbc/pull/480)
- `result_iterator`'s postfix increment returns `void`, so `*it++` is a compile error rather than the row after the one it names; `it++` and `++it` are unaffected.
- Cleared the remaining code scanning alerts. [`#476`](https://github.com/nanodbc/nanodbc/pull/476)
- Column buffers are owned by `std::unique_ptr`, and the implementation types are allocated with `std::make_shared`.
- The handle accessors that only read a member are now `noexcept`; in C++17 that is part of the function type, so a pointer to one needs it spelled out.
- `nanodbc.ruleset` and the Flawfinder job carry the analyser rules that have no answer here, each with its reason.

## v2.16.5

- handle SQL_NO_TOTAL when retrieving binary data in chunks [`#475`](https://github.com/nanodbc/nanodbc/pull/475), thanks [Jeroen Ooms](https://github.com/jeroen).

## v2.16.4

- Updating the usage, development, and contributing sections of generated docs to be in sync with the README and current codebase.

## v2.16.3

- Updated the README badges and links.
- Made sure the generated documentation is up to date with the current README and codebase.
- Removed the ancient build.bat file and fixed the .editorconfig file.
- Removed utility/vs2017 and utility/ci as well as they are no longer used.
- Add rstcheck to the CI/CD pipeline.

## v2.16.2

### Changes

- Small updates to README and documentation.

## v2.16.1

### Changes

- Automated the release process with GitHub Actions.

## v2.16.0

### Changes

- Updated to use Catch2 for testing.
- Increased test coverage to 80%.
- Fixed CI/CD integrations.
- Updated CMake build system.
- Fix for is_null() returning false for unbound columns when using SQLGetData().
- Fixed issue with nanodbc::time and nanodbc::timestamp not being able to be used as a parameter in a prepared statement when using SQL Server ODBC driver.
- Declare noexcept where a function really cannot throw.
- Updated code to follow the rule of five for classes that manage resources.
- Added dependabot configuration to automatically update dependencies.
- Updated clang-format version to the latest.
- Migrated test coverage to llvm-cov.
- Resolved many TODOs and FIXMEs in the codebase (most were out of date).

## v2.15.1

Added support for C++17 type std::optional with backward compatibility in C++14 via std::experimental::optional.

Added the one-byte integral types and `bool` to the explicit instantiations of `statement::bind`, `table_valued_parameter::bind`, `result::get` and `result::get_ref`, so that `signed char`, `unsigned char` and `bool` can be used when nanodbc is consumed as a library rather than failing to link [`#445`](https://github.com/nanodbc/nanodbc/discussions/445).

## v2.15.0

### New Features

- BREAKING: Public API function `complete` has been removed, use `complete_execute` instead [`#314`](https://github.com/nanodbc/nanodbc/pull/314).

### Changes

### Bug Fixes

### Acknowledgements

[Amy Troschinetz](https://github.com/lexicalunit), [Mateusz Loskot](https://github.com/mloskot),

## v2.14.0

### New Features

- Added Add `find_procedures` and `find_procedure_columns` to `catalog` class [`#249`](https://github.com/nanodbc/nanodbc/pull/249).
- Added support for binding `std::string_view` in `statement` class [`#283`](https://github.com/nanodbc/nanodbc/pull/283).

### Changes

- Changed return type of `result_impl::column_size` from `int` to `long` for consistency [`#261`](https://github.com/nanodbc/nanodbc/pull/261).
- Renamed `VERSION` file to `VERSION.txt` [`#275`](https://github.com/nanodbc/nanodbc/pull/275).

### Bug Fixes

- Fixed SQL statements in `example/usage.cpp` [`#253`](https://github.com/nanodbc/nanodbc/pull/253).
- Fixed `result_impl::column_datatype_name` sizing for Unicode characters [`#263`](https://github.com/nanodbc/nanodbc/pull/263).
- Fixed memory leak of `ensure_pdata` [`#269`](https://github.com/nanodbc/nanodbc/pull/269).
- Fixed retrieval of SQL data type `DATETIMEOFFSET` [`#219`](https://github.com/nanodbc/nanodbc/pull/219).
- Fixed compilation on MacOS using Homebrew's vanilla GCC (for Conan build) [`#279`](https://github.com/nanodbc/nanodbc/pull/279).

### Testing

- Add GitHub Actions with linters [`#273`](https://github.com/nanodbc/nanodbc/pull/273).

### Acknowledgements

Thank you to everyone who contributed to this release by committing changes and submitting pull requests:

[Amy Troschinetz](https://github.com/lexicalunit), [Bernardo Sulzbach](https://github.com/bernardosulzbach), [Denis Glazachev](https://github.com/traceon), [detule](https://github.com/detule), [Ezequiel Ruiz](https://github.com/emruiz81), [Joe Siltberg](https://github.com/joesiltberg), [Mateusz Loskot](https://github.com/mloskot), [Michael Kaes](https://github.com/mkaes), [Sewon Park](https://github.com/sphawk)

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

[Amy Troschinetz](https://github.com/lexicalunit), [Billy O'Neal](https://github.com/BillyONeal), [Christopher Blaesius](https://github.com/ChrisBFX), [Denis Glazachev](https://github.com/traceon), [detule](https://github.com/detule), [Diego Sogari](https://github.com/dsogari), [H1X4Dev](https://github.com/H1X4Dev), [Jim Hester](https://github.com/jimhester), [Jon Valvatne](https://github.com/jon-v), [Kun Ren](https://github.com/renkun-ken), [Mateusz Loskot](https://github.com/mloskot), [Michael C. Grant](https://github.com/mcg1969), [Rafee Memon](https://github.com/rafeememon), [Sauron](https://github.com/saur0n), [Seth Shelnutt](https://github.com/Shelnutt2), [ThermoX360](https://github.com/ThermoX360), [whizmo](https://github.com/whizmo)

## v2.12.4

Resolves a possible crash with `SQLDescribeParam()`. In Progress OpenEdge 11 driver setting the nullableptr argument to null causes a crash. This does not affect SQLite or MySQL drivers.

Thanks to [@AndrewJD79](https://github.com/AndrewJD79) for finding and diagnosing the issue!

## v2.12.3

Unicode: Resolves a major issue with BLOB datatype handling for BINARY and TEXT columns.

## v2.12.2

Resolves a major issue with BLOB datatype handling for BINARY and TEXT columns.

## v2.12.1

Resolves a Travis-CI build issue.

## v2.12.0

Major work undertaken by Mateusz Łoskot provides new features and a host of bug fixes throughout. Refactoring work moves nanodbc away from platform dependent `wchar_t` in favor of `char16_t` or in the case of iODBC with Unicode build enabled, `char32_t`. Boost.Test dropped in this version, in favor of Catch.

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
    - ${name}\_test: runs ${name}\_test, but will not build it
    - ${name}\_tests: builds ${name}\_test, but does not run it
    - ${name}\_check: builds ${name}\_test and then runs it

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
