set(BINARY ${CMAKE_PROJECT_NAME}_tst)

# file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)
set(TEST_SOURCES 
asynctcpsocket_unittest.cc
asyncudpsocket_unittest.cc
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
linux_unittest.cc
# logging_unittest.cc
main.cpp
network_unittest.cc
pathutils_unittest.cc
signalthread_unittest.cc
sigslot_unittest.cc
# stream_unittest.cc
stringencode_unittest.cc
stringutils_unittest.cc
# systeminfo_unittest.cc
task_unittest.cc
testclient_unittest.cc
timeutils_unittest.cc
versionparsing_unittest.cc
# virtualsocket_unittest.cc #Test is ok but it takes so much time
)
set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)