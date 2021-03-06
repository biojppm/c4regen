cmake_minimum_required(VERSION 3.2)
project(c4regen)

include(./ext/c4core/cmake/c4Project.cmake)
c4_declare_project(c4regen)

find_library(LIBCLANG_LIB clang)
find_path(LIBCLANG_INC clang-c/Index.h)
if(NOT LIBCLANG_INC)
    c4_err("could not find clang-c/Index.h. Please set the variable: LIBCLANG_INC")
endif()
if(NOT LIBCLANG_LIB)
    c4_err("could not find libclang. Please set the variable: LIBCLANG_LIB")
endif()
if(WIN32)
    find_file(LIBCLANG_DLL libclang.dll)
    if(NOT LIBCLANG_DLL)
        c4_err("could not find libclang.dll. Please set the variable: LIBCLANG_DLL")
    endif()
endif()


c4_require_subproject(c4core    SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4core)
c4_require_subproject(c4fs      SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4fs)
c4_require_subproject(c4log     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4log)
c4_require_subproject(c4opt     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4opt)
c4_require_subproject(ryml      SUBDIRECTORY ${C4REGEN_EXT_DIR}/rapidyaml)
c4_require_subproject(c4tpl     SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4tpl)

c4_add_library(c4regen
    LIBS c4fs c4tpl ryml c4log c4fs c4opt c4core ${LIBCLANG_LIB}
    DLLS ${LIBCLANG_DLL}
    INC_DIRS
       $<BUILD_INTERFACE:${C4REGEN_SRC_DIR}> $<INSTALL_INTERFACE:include>
       $<BUILD_INTERFACE:${LIBCLANG_INC}>
    SOURCE_ROOT ${C4REGEN_SRC_DIR}
    SOURCES
        c4/ast/ast.hpp
        c4/ast/ast.cpp
        c4/regen/class.hpp
        c4/regen/class.cpp
        c4/regen/exec.hpp
        c4/regen/entity.hpp
        c4/regen/entity.cpp
        c4/regen/enum.hpp
        c4/regen/enum.cpp
        c4/regen/extractor.hpp
        c4/regen/extractor.cpp
        c4/regen/function.hpp
        c4/regen/function.cpp
        c4/regen/generator.hpp
        c4/regen/generator.cpp
        c4/regen/regen.hpp
        c4/regen/regen.cpp
        c4/regen/source_file.hpp
        c4/regen/source_file.cpp
        c4/regen/writer.hpp
        c4/regen/writer.cpp
)

c4_add_executable(regen
    SOURCES main.cpp SOURCE_ROOT ${C4REGEN_SRC_DIR}
    LIBS c4regen
)

c4_install_target(c4regen)
c4_install_target(regen)
c4_install_exports()

c4_add_dev_targets()
