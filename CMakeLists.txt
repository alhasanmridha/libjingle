cmake_minimum_required(VERSION 3.9.2)
project(Libjigle)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_subdirectory(src)
add_subdirectory(googletest)
