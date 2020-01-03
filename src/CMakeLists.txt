set(BINARY ${CMAKE_PROJECT_NAME})
# set(CMAKE_VERBOSE_MAKEFILE ON)
# file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES
asyncfile.cc
asyncsocket.cc
base64.cc
bytebuffer.cc
event.cc
fileutils.cc
helpers.cc
host.cc
ipaddress.cc
logging.cc
main.cpp
messagehandler.cc
messagequeue.cc
nethelpers.cc
pathutils.cc
physicalsocketserver.cc
signalthread.cc
socketaddress.cc
stream.cc
stringutils.cc
stringencode.cc
thread.cc
timeutils.cc
unixfilesystem.cc
urlencode.cc
versionparsing.cc
)
set(SOURCES ${SOURCES})
# add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})