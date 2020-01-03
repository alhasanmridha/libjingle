set(BINARY ${CMAKE_PROJECT_NAME}_tst)

# file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)
set(TEST_SOURCES 
base64_unittest.cc
basictypes_unittest.cc
bind_unittest.cc
buffer_unittest.cc
bytebuffer_unittest.cc
byteorder_unittest.cc
event_unittest.cc
# fileutils_unittest.cc
# helpers_unittest.cc
host_unittest.cc
# logging_unittest.cc
main.cpp
pathutils_unittest.cc
signalthread_unittest.cc
sigslot_unittest.cc
stringencode_unittest.cc
stringutils_unittest.cc
timeutils_unittest.cc
versionparsing_unittest.cc
)
set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)