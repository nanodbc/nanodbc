@echo off
rem Runs CMake to configure nanodbc (static library) for Visual Studio 2017.
rem Runs MSBuild to build the generated solution.
rem
rem Usage:
rem 1. Copy build.bat to build.locale.bat (git ignored file)
rem 2. Make your adjustments in the CONFIGURATION section below
rem 3. Run build.local.bat 32|64
rem 4. Optionally, run devenv.exe nanodbc{32|64}.sln from command line,
rem    so it will pick up environment variables (e.g. NANODBC_TEST_CONNSTR)
rem    This will allow to run tests conveniently, without editing projects.

rem ### CONFIGURATION #####################################
rem ### Connection strings for tests (alternatively, use command line-c option)
rem ### For example, SQL Server LocalDB instance, MySQL and PostgreSQL on the nanodbc Vagrant VM.
rem set NANODBC_TEST_CONNSTR_MSSQL=Driver={ODBC Driver 11 for SQL Server};Server=(localdb)\MSSQL13DEV;Integrated Security=True;Database=nanodbc;MARS_Connection=Yes;
rem Connection strings for nanodbc Vagrant environment
set NANODBC_TEST_CONNSTR_MSSQL=Driver={ODBC Driver 11 for SQL Server};Server=localhost,2433;Database=vagrant;UID=vagrant;PWD=vagrant;
set NANODBC_TEST_CONNSTR_MYSQL=Driver={MySQL ODBC 5.3 Unicode Driver};Server=localhost;Port=4306;Database=vagrant;User=vagrant;Password=vagrant;Option=3;
set NANODBC_TEST_CONNSTR_PGSQL=Driver={PostgreSQL Unicode(x64)};Server=localhost;Port=6432;Database=vagrant;UID=vagrant;PWD=vagrant;
setlocal
set NANODBC_DISABLE_INSTALL=ON
set BOOST_ROOT=C:/local/boost_1_59_0
rem #######################################################

if not defined VS150COMNTOOLS goto :NoVS
if [%1]==[] goto :Usage

set NANOU=""
if /I "%2"=="U"  set NANOU=U
if [%1]==[32] goto :32
if [%1]==[64] goto :64
goto :Usage

:32
set NANOP=32
set MSBUILDP=Win32
set GENERATOR="Visual Studio 15 2017"
goto :Build

:64
set NANOP=64
set MSBUILDP=x64
set GENERATOR="Visual Studio 15 2017 Win64"
goto :Build

:Build
set NANODBC_ENABLE_UNICODE=OFF
if /I "%NANOU%"=="U" set NANODBC_ENABLE_UNICODE=ON
set BUILDDIR=_build%NANOP%%NANOU%
mkdir %BUILDDIR%
pushd %BUILDDIR%
"C:\Program Files\CMake\bin\cmake.exe" ^
    -G %GENERATOR% ^
    -DNANODBC_ENABLE_UNICODE=%NANODBC_ENABLE_UNICODE% ^
    -DNANODBC_DISABLE_INSTALL=%NANODBC_DISABLE_INSTALL% ^
    -DBOOST_ROOT:PATH=%BOOST_ROOT% ^
    -DBOOST_LIBRARYDIR:PATH=%BOOST_ROOT%/lib%NANOP%-msvc-14.0 ^
    ..
move nanodbc.sln nanodbc%NANOP%%NANOU%.sln
rem Build
msbuild.exe nanodbc%NANOP%%NANOU%.sln /p:Configuration=Release /p:Platform=%MSBUILDP%
rem Load with Visual Studio Selector
nanodbc%NANOP%%NANOU%.sln
popd
goto :EOF

:NoVS
@echo build.bat
@echo  Visual Studio 2017 not found
@echo  "%%VS150COMNTOOLS%%" environment variable not defined
exit /B 1

:Usage
@echo build.bat
@echo Usage: build.bat (32 or 64) [U]
@echo        Optional U for Unicode build
exit /B 1
