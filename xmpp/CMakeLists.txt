set(BINARY ${CMAKE_PROJECT_NAME}_xmpp)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false *.cc)

add_library(${BINARY} STATIC ${SOURCES})

target_link_libraries(${BINARY} PUBLIC 
${CMAKE_PROJECT_NAME}_base 
${CMAKE_PROJECT_NAME}_xmllite
gtest
expat
)