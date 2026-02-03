set(TARGET_NAME coro_echo)

add_executable(${TARGET_NAME}
    echo_server.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE Boost::boost coro_cxx::common)
