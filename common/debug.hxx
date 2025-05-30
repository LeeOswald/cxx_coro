#pragma once

#ifndef CXX_CORO_COMMON_HXX_INCLUDED
#include "common.hxx"
#endif

#include <cassert>
#include <functional>
#include <string>

#include <fmt/format.h>


namespace cxx_coro
{

enum class Level
{
    Verbose,
    Info,
    Error
};

using TraceFn = std::function<void(Level level, std::uint32_t indent, std::string_view Message)>;


CXX_CORO_EXPORT TraceFn setTracer(TraceFn&& f); // NOT thread-safe

CXX_CORO_EXPORT void writeln(Level level, std::string_view message);

template <class... Args>
void write(Level level, std::string_view format, Args&&... args)
{
    writeln(level, fmt::vformat(format, fmt::make_format_args(args...)));
}

template <class... Args>
void verbose(std::string_view format, Args&&... args)
{
    writeln(Level::Verbose, fmt::vformat(format, fmt::make_format_args(args...)));
}

template <class... Args>
void info(std::string_view format, Args&&... args)
{
    writeln(Level::Info, fmt::vformat(format, fmt::make_format_args(args...)));
}

template <class... Args>
void error(std::string_view format, Args&&... args)
{
    writeln(Level::Error, fmt::vformat(format, fmt::make_format_args(args...)));
}


struct CXX_CORO_EXPORT IndentScope
{
    ~IndentScope()
    {
        unindent();
    }

    template <class... Args>
    IndentScope(Level level, std::string_view format, Args&&... args)
    {
        writeln(level, fmt::vformat(format, fmt::make_format_args(args...)));

        indent();
    }

private:
    void indent() noexcept;
    void unindent() noexcept;
};


CXX_CORO_EXPORT std::string binaryToHex(std::string_view binary);
CXX_CORO_EXPORT std::string binaryToAscii(std::string_view binary);

} // namespace cxx_coro {}


#if CXX_CORO_ENABLE_LOG

#if CXX_CORO_VERBOSE_LOG

#define Verbose(format, ...) \
    ::cxx_coro::verbose(format, ##__VA_ARGS__)


#define  VerboseBlock(format, ...) \
    ::cxx_coro::IndentScope __is(::cxx_coro::Level::Verbose, format, ##__VA_ARGS__)

#else

#define Verbose(format, ...)                 ((void)0)
#define VerboseBlock(format, ...)            ((void)0)

#endif

#define Info(format, ...) \
    ::cxx_coro::info(format, ##__VA_ARGS__)

#define  InfoBlock(format, ...) \
    ::cxx_coro::IndentScope __is(::cxx_coro::Level::Info, format, ##__VA_ARGS__)


#define Error(format, ...) \
    ::cxx_coro::error(format, ##__VA_ARGS__)

#define  ErrorBlock(format, ...) \
    ::cxx_coro::IndentScope __is(::cxx_coro::Level::Error, format, ##__VA_ARGS__)

#else // !CXX_CORO_ENABLE_LOG

#define Verbose(format, ...)                 ((void)0)
#define VerboseBlock(format, ...)            ((void)0)

#define Info(format, ...)                    ((void)0)
#define InfoBlock(format, ...)               ((void)0)

#define Error(format, ...)                   ((void)0)
#define ErrorBlock(format, ...)              ((void)0)

#endif // !CXX_CORO_ENABLE_LOG


#if defined __clang__ || defined __GNUC__
#define CXX_CORO_FUNCTION __PRETTY_FUNCTION__
#elif defined _MSC_VER
#define CXX_CORO_FUNCTION __FUNCSIG__
#endif