set(CMAKE_C_FLAGS "-std=gnu11 -pthread -O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -pthread -O3 ${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(cpu_client main.cpp)
target_link_libraries(cpu_client rt pthread)
install(TARGETS cpu_client DESTINATION "${CMAKE_INSTALL_BINDIR}")

function(add_client_alias name)
  add_custom_command(OUTPUT ${name}
    COMMAND "${CMAKE_COMMAND}" -E create_symlink cpu_client
    "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  add_custom_target(${name}.target ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
endfunction()

add_client_alias(schedule_rr)
add_client_alias(rr_wrapper)
