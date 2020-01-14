set(BINARY ${CMAKE_PROJECT_NAME}_chat)
add_executable(${BINARY} 
chat_main.cc
chatapp.cc
consoletask.cc
textchatreceivetask.cc
textchatsendtask.cc
)
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_engine ${CMAKE_PROJECT_NAME}_xmllite)