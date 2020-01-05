set(BINARY ${CMAKE_PROJECT_NAME}_xmllite_test)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false *.h *.cpp *.cc)

add_executable(${BINARY} ${SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC 
${CMAKE_PROJECT_NAME}_base
${CMAKE_PROJECT_NAME}_xmllite
gtest
expat
)