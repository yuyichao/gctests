set(CMAKE_C_FLAGS "-std=gnu11 -pthread -O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -pthread -O3 ${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(cpu_server main.cpp)
target_link_libraries(cpu_server rt pthread)
