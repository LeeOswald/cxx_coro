set(TARGET_NAME common)

add_library(${TARGET_NAME} STATIC
    common.hxx
    debug.hxx
    debug.cxx
)

target_link_libraries(${TARGET_NAME} PUBLIC fmt::fmt)