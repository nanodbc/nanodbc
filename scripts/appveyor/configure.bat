@echo off
rem Runs CMake to configure nanodbc (static) for Visual Studio 2015.
rem Runs MSBuild to build the generated solution.
setlocal

IF /I NOT "%APPVEYOR%"=="True" (
    @ECHO   This script is dedicate to be run on AppVeyor
    EXIT /B 1
)
rem #######################################################
set NANODBC_STATIC=ON
set NANODBC_INSTALL=OFF
set GENERATOR="Visual Studio 14 2015 Win64"
rem set BOOST_ROOT=C:/local/boost_1_59_0
rem #######################################################

cmake.exe ^
    -G %GENERATOR% ^
    -DNANODBC_STATIC=%NANODBC_STATIC% ^
    -DNANODBC_USE_BOOST_CONVERT=%USE_BOOST_CONVERT% ^
    -DNANODBC_USE_UNICODE=%USE_UNICODE% ^
    -DNANODBC_HANDLE_NODATA_BUG=%USE_NODATA_BUG% ^
    -DNANODBC_TEST=ON ^
    -DNANODBC_INSTALL=%NANODBC_INSTALL% ^
    %APPVEYOR_BUILD_FOLDER%
