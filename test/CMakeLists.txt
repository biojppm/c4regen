c4_setup_testing()

function(c4regen_add_test name)
    c4_add_executable(c4regen-test-${name}
        SOURCES ${ARGN} main.cpp
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        DLLS ${LIBCLANG_DLL}   # FIXME transitive
        LIBS c4regen gtest
        FOLDER test)
    c4_add_test(c4regen-test-${name})
endfunction(c4regen_add_test)

c4regen_add_test(basic basic.cpp)
