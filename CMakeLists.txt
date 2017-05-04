cmake_minimum_required(VERSION 3.0)
project(explore)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu11 -pthread -g -O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -pthread -g -O3 ${CMAKE_CXX_FLAGS}")

add_definitions("-D_GNU_SOURCE")
set(exec_perm OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
function(install_bin_exec file rename)
  install(FILES "${file}" DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RENAME "${rename}"
    PERMISSIONS ${exec_perm})
endfunction()

add_subdirectory(cpu_server)
add_subdirectory(cpu_client)
add_subdirectory(scripts)
