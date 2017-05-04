cmake_minimum_required(VERSION 2.8)
project(explore)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu11 -pthread -g -O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -pthread -g -O3 ${CMAKE_CXX_FLAGS}")

add_definitions("-D_GNU_SOURCE")

add_subdirectory(cpu_server)
add_subdirectory(cpu_client)
