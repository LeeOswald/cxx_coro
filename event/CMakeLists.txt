set(TARGET_NAME event)

add_executable(${TARGET_NAME}
    event.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE coro_cxx::common)