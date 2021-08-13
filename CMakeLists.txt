
cmake_minimum_required(VERSION 3.15)
project(influxm)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

option(LIB_INFLUXDB_TESTING "Enable test building" OFF)

include(Influxdb.cmake)

if (LIB_INFLUXDB_TESTING)
add_subdirectory(test)
endif()
