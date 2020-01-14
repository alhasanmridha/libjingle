set(BINARY ${CMAKE_PROJECT_NAME}_login)
add_executable(${BINARY} login_main.cc)
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_engine ${CMAKE_PROJECT_NAME}_xmllite ${CMAKE_PROJECT_NAME}_base)