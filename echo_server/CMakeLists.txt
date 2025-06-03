set(TARGET_NAME echo_server)

add_executable(${TARGET_NAME}
    echo_server.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE boost::boost coro_cxx::common)