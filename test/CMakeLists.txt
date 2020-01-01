set(BINARY ${CMAKE_PROJECT_NAME}_tst)

# file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)
set(TEST_SOURCES 
basictypes_unittest.cc
byteorder_unittest.cc
stringutils_unittest.cc
bytebuffer_unittest.cc
stringencode_unittest.cc
buffer_unittest.cc
sigslot_unittest.cc
main.cpp
)
set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)