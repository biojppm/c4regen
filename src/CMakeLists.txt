set(src
    c4/ast/ast.hpp
    c4/ast/ast.cpp
    c4/regen/regen.hpp
    c4/regen/regen.cpp
)

find_library(LIBCLANG_LIB clang)
find_path(LIBCLANG_INC clang-c/Index.h)
if((NOT LIBCLANG_LIB) OR (NOT LIBCLANG_INC))
    message(FATAL_ERROR "could not find libclang")
endif()

c4_require_module(C4REGEN c4core    SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4core)
c4_require_module(C4REGEN rapidyaml SUBDIRECTORY ${C4REGEN_EXT_DIR}/rapidyaml)
c4_require_module(C4REGEN c4tpl     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4tpl)
c4_require_module(C4REGEN c4opt     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4opt)
c4_require_module(C4REGEN c4log     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4log)
c4_require_module(C4REGEN c4fs      SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4fs)

c4_add_library(C4REGEN c4regen
    SOURCES ${src}
    LIBS c4fs c4tpl c4log ${LIBCLANG_LIB}
    INTERFACES c4opt
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR} ${LIBCLANG_INC}
)

c4_add_executable(C4REGEN regen
    SOURCES main.cpp
    LIBS c4regen
    INC_DIRS ${C4REGEN_EXT_DIR}/optionparser/src)
