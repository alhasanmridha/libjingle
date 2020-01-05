set(BINARY ${CMAKE_PROJECT_NAME}_xmllite)

file(GLOB_RECURSE XMLLITE_SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)

add_library(${BINARY} STATIC ${XMLLITE_SOURCES})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_base)