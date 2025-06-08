set(TARGET_NAME cancel)

add_executable(${TARGET_NAME}
    cancel.hxx
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE boost::boost coro_cxx::common)