@echo off
rem Runs CMake to configure nanodbc (static library) for Visual Studio 2015.
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
setlocal
set NANODBC_STATIC=ON
set NANODBC_INSTALL=OFF
set BOOST_ROOT=C:/local/boost_1_59_0
endlocal
rem ### Connection strings for tests (alternatively, use command line-c option)
rem ### For example, SQL Server LocalDB instance, MySQL and PostgreSQL on the nanodbc Vagrant VM.
set NANODBC_TEST_CONNSTR_MSSQL=Driver={ODBC Driver 11 for SQL Server};Server=(localdb)\MSSQL13DEV;Integrated Security=True;Database=nanodbc;MARS_Connection=Yes;
set NANODBC_TEST_CONNSTR_MYSQL=Driver={MySQL ODBC 5.3 Unicode Driver};Database=vagrant;Server=localhost;User=vagrant;Password=vagrant;Option=3;
set NANODBC_TEST_CONNSTR_PGSQL=Driver={PostgreSQL Unicode(x64)};Server=localhost;Database=vagrant;UID=vagrant;PWD=vagrant;
rem #######################################################

setlocal

if not defined VS140COMNTOOLS goto :NoVS
if [%1]==[] goto :Usage
if [%1]==[32] goto :32
if [%1]==[64] goto :64
goto :Usage

:32
set NANOP=32
set MSBUILDP=Win32
set GENERATOR="Visual Studio 14 2015"
goto :Build

:64
set NANOP=64
set MSBUILDP=x64
set GENERATOR="Visual Studio 14 2015 Win64"
goto :Build

:Build
set BUILDDIR=_build%NANOP%
mkdir %BUILDDIR%
pushd %BUILDDIR%
"C:\Program Files\CMake\bin\cmake.exe" ^
    -G %GENERATOR% ^
    -DNANODBC_STATIC=%NANODBC_STATIC% ^
    -DNANODBC_USE_UNICODE=ON ^
    -DNANODBC_TEST=ON ^
    -DNANODBC_INSTALL=%NANODBC_INSTALL% ^
    -DBOOST_ROOT:PATH=%BOOST_ROOT% ^
    -DBOOST_LIBRARYDIR:PATH=%BOOST_ROOT%/lib%NANOP%-msvc-14.0 ^
    ..
move nanodbc.sln nanodbc%NANOP%.sln
msbuild.exe nanodbc%NANOP%.sln /p:Configuration=Release /p:Platform=%MSBUILDP%
popd
goto :EOF

:NoVS
@echo build.bat
@echo  Visual Studio 2015 not found
@echo  "%%VS140COMNTOOLS%%" environment variable not defined
exit /B 1

:Usage
@echo build.bat
@echo Usage: build.bat [32 or 64]
exit /B 1
