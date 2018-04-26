cmake_minimum_required(VERSION 3.2)
project(c4regen)

include(./extern/c4tpl/cmake/c4Project.cmake)
c4_declare_project(c4regen)

set(C4REGEN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(C4REGEN_EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

add_subdirectory(src)

if(C4REGEN_BUILD_TESTS)
    enable_testing() # this must be done here (and not inside the test dir)
                     # so that the test targets are available at the top level
    add_subdirectory(test)
endif()

if(C4REGEN_BUILD_BENCHMARKS)
    add_subdirectory(bm)
endif()
