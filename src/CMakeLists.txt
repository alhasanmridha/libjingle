set(BINARY ${CMAKE_PROJECT_NAME})
# set(CMAKE_VERBOSE_MAKEFILE ON)
# file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES
stringutils.cc
bytebuffer.cc
stringencode.cc
versionparsing.cc
host.cc
asyncfile.cc
logging.cc
timeutils.cc
stream.cc
thread.cc
messagehandler.cc
messagequeue.cc
physicalsocketserver.cc
signalthread.cc
socketaddress.cc
asyncsocket.cc
main.cpp
)
set(SOURCES ${SOURCES})
set(CMAKE_CXX_FLAGS "-pthread")
add_definitions(-DPOSIX=5)
add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})