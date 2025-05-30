set(TARGET_NAME generator)

add_executable(${TARGET_NAME}
    generator.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE coro_cxx::common)