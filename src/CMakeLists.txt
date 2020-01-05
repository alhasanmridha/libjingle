set(BINARY ${CMAKE_PROJECT_NAME})
# set(CMAKE_VERBOSE_MAKEFILE ON)
# file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES
asyncfile.cc
asynchttprequest
asyncsocket.cc
asynctcpsocket.cc
asyncudpsocket.cc
autodetectproxy.cc
bandwidthsmoother.cc
base64.cc
bytebuffer.cc
diskcache.cc
checks.cc
cpumonitor.cc
crc32.cc
event.cc
fileutils.cc
filelock.cc
flags.cc
helpers.cc
host.cc
httpbase.cc
httpclient.cc
httpcommon.cc
httpserver.cc
ipaddress.cc
linux.cc
linuxfdwalk.c
logging.cc
main.cpp
messagedigest.cc
messagehandler.cc
messagequeue.cc
nethelpers.cc
network.cc
openssladapter.cc
openssldigest.cc
optionsfile.cc
pathutils.cc
physicalsocketserver.cc
posix.cc
profiler.cc
proxydetect.cc
proxyinfo.cc
proxyserver.cc
ratelimiter.cc
signalthread.cc
socketadapters.cc
socketaddress.cc
socketaddresspair.cc
socketpool.cc
socketstream.cc
ssladapter.cc
sslsocketfactory.cc
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
timing.cc
unixfilesystem.cc
urlencode.cc
versionparsing.cc
virtualsocketserver.cc
)
set(SOURCES ${SOURCES})
# add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})
find_package(OpenSSL REQUIRED)
target_link_libraries(${BINARY}_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)