set(src
    c4/ast/ast.hpp
    c4/ast/ast.cpp
    c4/regen/regen.hpp
    c4/regen/regen.cpp
)

find_library(LIBCLANG_LIB clang)
find_path(LIBCLANG_INC clang-c/Index.h)

c4_require_module(C4REGEN c4tpl SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4tpl)

c4_add_library(C4REGEN c4regen
    SOURCES ${src}
    LIBS c4tpl ${LIBCLANG_LIB}
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR} ${LIBCLANG_INC}
)

c4_add_executable(C4REGEN regen
    SOURCES main.cpp
    LIBS c4regen)
