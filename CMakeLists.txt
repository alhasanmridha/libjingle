cmake_minimum_required(VERSION 3.5.1)
project(libjigle)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(googletest)
