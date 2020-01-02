cmake_minimum_required(VERSION 3.5.1)
project(libjigle)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-pthread -lssl -lcrypto")
add_definitions(-DPOSIX=5 -DHAVE_OPENSSL_SSL_H=1)

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(googletest)
