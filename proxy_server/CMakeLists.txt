set(TARGET_NAME proxy_server)

add_executable(${TARGET_NAME}
    proxy_server.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE boost::boost coro_cxx::common)