set(BINARY ${CMAKE_PROJECT_NAME}_xmllite)

file(GLOB_RECURSE XMLLITE_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)

set(SOURCES ${XMLLITE_SOURCES})

add_library(${BINARY} STATIC ${SOURCES})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)