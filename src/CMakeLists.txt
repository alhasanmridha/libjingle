set(BINARY ${CMAKE_PROJECT_NAME})
# set(CMAKE_VERBOSE_MAKEFILE ON)
# file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES
asyncfile.cc
asyncsocket.cc
asynctcpsocket.cc
asyncudpsocket.cc
base64.cc
bytebuffer.cc
event.cc
fileutils.cc
filelock.cc
helpers.cc
host.cc
ipaddress.cc
linux.cc
logging.cc
main.cpp
messagehandler.cc
messagequeue.cc
nethelpers.cc
network.cc
pathutils.cc
physicalsocketserver.cc
profiler.cc
signalthread.cc
socketaddress.cc
socketaddresspair.cc
stream.cc
stringutils.cc
stringencode.cc
systeminfo.cc
task.cc
taskparent.cc
taskrunner.cc
testclient.cc
thread.cc
timeutils.cc
unixfilesystem.cc
urlencode.cc
versionparsing.cc
virtualsocketserver.cc
)
set(SOURCES ${SOURCES})
# add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})