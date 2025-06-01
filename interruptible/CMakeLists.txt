set(TARGET_NAME interruptible)

add_executable(${TARGET_NAME}
    interruptible.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE boost::boost coro_cxx::common)