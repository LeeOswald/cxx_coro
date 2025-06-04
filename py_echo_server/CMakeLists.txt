set(TARGET_NAME Echo)

Python3_add_library(${TARGET_NAME} MODULE py_echo_server.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(${TARGET_NAME} PRIVATE boost::boost coro_cxx::common)