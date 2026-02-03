set(TARGET_NAME asio_dispatch)

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE Boost::boost coro_cxx::common)
