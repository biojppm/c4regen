set(src
    c4/regen/regen.hpp
    c4/regen/regen.cpp
)

add_subdirectory(${C4REGEN_EXT_DIR}/cppast cppast)

c4_require_module(C4REGEN c4tpl SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4tpl)

c4_add_library(C4REGEN c4regen
    SOURCES ${src}
    LIBS c4tpl cppast
    INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
)

c4_add_executable(C4REGEN regen
    SOURCES main.cpp
    LIBS c4regen)
