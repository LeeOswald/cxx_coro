cmake_minimum_required(VERSION 3.25)


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build configuration specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(CXX_CORO_ENABLE_LOG "Enable logging" ON)
if(CXX_CORO_ENABLE_LOG)
    add_definitions(-DCXX_CORO_ENABLE_LOG=1)
endif()

option(CXX_CORO_VERBOSE_LOG "Enable verbose logging" OFF)
if(CXX_CORO_VERBOSE_LOG)
    add_definitions(-DCXX_CORO_VERBOSE_LOG=1)
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(cxx-coro VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


# detect OS
if(WIN32)
    set(CXX_CORO_WINDOWS TRUE)
    add_definitions(-DCXX_CORO_WINDOWS=1)
elseif(UNIX AND NOT APPLE)
    set(CXX_CORO_LINUX TRUE)
    add_definitions(-DCXX_CORO_LINUX=1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

# Boost
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# {fmt}
find_package(fmt REQUIRED)

find_package(Python3 3 REQUIRED COMPONENTS Development.Module)

if(MSVC)
    add_compile_options("/utf-8")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
)

# Windows-specific settings
if(CXX_CORO_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0602)
endif()

add_subdirectory(common)
add_library(coro_cxx::common ALIAS common)

add_subdirectory(asio_coro)
add_subdirectory(asio_dispatch)
add_subdirectory(generator)
add_subdirectory(echo_server)
add_subdirectory(event)
add_subdirectory(interruptible)
add_subdirectory(proxy_server)
add_subdirectory(py_echo_server)
add_subdirectory(std_generator)