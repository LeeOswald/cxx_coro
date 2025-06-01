#include "common.hxx"

#include <cctype>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <thread>

#if CXX_CORO_WINDOWS
    #include <windows.h>
#endif

namespace cxx_coro
{

namespace
{

void defaultTracer(Level level, std::uint32_t indent, std::string_view message)
{
    auto tid = std::this_thread::get_id();

    std::ostringstream ss;
    switch (level)
    {
    case cxx_coro::Level::Verbose: ss << "V "; break;
    case cxx_coro::Level::Info: ss << "I "; break;
    case cxx_coro::Level::Error: ss << "E "; break;
    }

    ss << "@" << tid << " | ";

    while (indent)
    {
        ss << "    ";
        --indent;
    }

    ss << message << "\n";

    std::string msg(ss.str());

    if (level < Level::Error)
        std::osyncstream(std::cout) << msg;
    else
        std::osyncstream(std::cerr) << msg;

#if CXX_CORO_WINDOWS
    if (::IsDebuggerPresent())
    {
        std::wstring out;

        auto required = ::MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.length()), nullptr, 0);
        if (required > 0)
        {
            out.resize(required);
            ::MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.length()), out.data(), static_cast<int>(out.size()));
        }

        ::OutputDebugStringW(out.c_str());
    }
#endif
}

TraceFn g_Tracer = defaultTracer;

thread_local std::uint32_t g_indent = 0;

constexpr std::uint32_t MaxIndent = 32;

} // namespace {}


void IndentScope::indent() noexcept
{
    if (g_indent < MaxIndent)
        ++g_indent;
}

void IndentScope::unindent() noexcept
{
    if (g_indent > 0)
        --g_indent;
}


CXX_CORO_EXPORT TraceFn setTracer(TraceFn&& f)
{
    auto prev = std::move(g_Tracer);

    if (!f)
        g_Tracer = defaultTracer;
    else
        g_Tracer = std::move(f);

    return prev;
}

CXX_CORO_EXPORT void writeln(Level level, std::string_view message)
{
    g_Tracer(level, g_indent, message);
}

CXX_CORO_EXPORT std::string binaryToHex(std::string_view binary)
{
    auto halfByteToStr = [](std::uint8_t half) -> char
    {
        static const char HexChars[16] =
        {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        };

        assert(half < 16);
        return HexChars[half];
    };

    auto p = reinterpret_cast<const std::uint8_t*>(binary.data());
    auto len = binary.size();

    std::ostringstream ss;
    bool first = true;

    while (len)
    {
        if (first)
            first = false;
        else
            ss << ' ';

        auto hi = (*p) >> 4;
        auto lo = (*p) & 0x0f;

        ss << halfByteToStr(hi) << halfByteToStr(lo);

        ++p;
        --len;
    }

    return ss.str();
}

CXX_CORO_EXPORT std::string binaryToAscii(std::string_view binary)
{
    auto p = reinterpret_cast<const std::uint8_t*>(binary.data());
    auto len = binary.size();

    std::ostringstream ss;

    while (len)
    {
        if (std::isprint(*p))
            ss << static_cast<char>(*p);
        else
            ss << '.';

        ++p;
        --len;
    }

    return ss.str();
}


} // namespace cxx_coro {}
