set(src
    c4/regen/regen.hpp
    c4/regen/regen.cpp
)

c4_require_module(c4regen
    c4tpl SUBDIRECTORY ${C4REGEN_EXT_DIR}/c4tpl)

c4_add_library(C4REGEN c4regen
    SOURCES ${src}
    LIBS c4tpl
    INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
)

c4_add_executable(C4REGEN regen
    SOURCES main.cpp
    LIBS c4regen)
