set(TARGET_NAME std_generator)

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE coro_cxx::common)