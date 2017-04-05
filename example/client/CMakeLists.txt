cmake_minimum_required(VERSION 3.0.0)
project(nanodbc-client CXX)

if (NOT TARGET nanodbc)
  find_package(nanodbc CONFIG REQUIRED)
endif()

message(STATUS "nanodbc package found - ${nanodbc_FOUND}")
message(STATUS "nanodbc package directory - ${nanodbc_DIR}")

add_executable(nanodbc_client nanodbc_client.cpp)
target_link_libraries(nanodbc_client nanodbc)
