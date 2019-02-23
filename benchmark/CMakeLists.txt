cmake_minimum_required(VERSION 3.11)
project(code)

set(CMAKE_CXX_STANDARD 17)

add_library(libbenchmark STATIC IMPORTED)
set_property(TARGET libbenchmark PROPERTY IMPORTED_LOCATION /usr/local/lib/libbenchmark.a)

set( CMAKE_VERBOSE_MAKEFILE on )

set(CMAKE_SHARED_LINKER_FLAGS "-pthread -Wl,-no-as-needed")
set(CMAKE_CXX_FLAGS " -O3 -Wall -pedantic -march=native")

add_executable(code
        main.cpp)

target_link_libraries(code libbenchmark pthread rt)
