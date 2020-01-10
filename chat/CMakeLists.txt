set(BINARY ${CMAKE_PROJECT_NAME}_chat)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false *.cc)

add_executable(${BINARY} ${SOURCES})

target_link_libraries(${BINARY} PUBLIC 
${CMAKE_PROJECT_NAME}_base 
${CMAKE_PROJECT_NAME}_xmpp
)