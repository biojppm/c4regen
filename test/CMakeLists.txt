c4_setup_testing(C4REGEN ON)

function(c4regen_add_test name)
    c4_add_executable(C4REGEN c4regen-test-${name}
        SOURCES ${ARGN} main.cpp
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        LIBS c4regen gtest
        FOLDER c4regen-test)
    c4_add_test(C4REGEN c4regen-test-${name} ON)
endfunction(c4regen_add_test)

#c4regen_add_test(rope test_regen.cpp)
