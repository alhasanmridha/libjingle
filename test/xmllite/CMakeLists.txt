set(BINARY ${CMAKE_PROJECT_NAME}_xmllite_tst)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)

set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC 
${CMAKE_PROJECT_NAME}_lib
${CMAKE_PROJECT_NAME}_xmllite
gtest
expat
)