#pragma once

#include "common.hxx"

#include <array>
#include <bit>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

extern "C" PyAPI_FUNC(int) _PyArg_ParseStackAndKeywords(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames,
    struct _PyArg_Parser *,
    ...);

namespace util
{

constexpr auto nbeswap(std::integral auto val) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
        return val;

    return std::byteswap(val);
}

} // namespace util {}


boost::asio::awaitable<void> client_handler(boost::asio::ip::tcp::socket s, PyObject* app) 
{
    VerboseBlock("client_handler()");

    PyObject* appReturn{ nullptr };

    try 
    {
        std::vector<char> data;
        auto vecCall{ PyVectorcall_Function(app) };

        for (;;) 
        {
            Verbose("receiving...");
            std::uint16_t hdr = 0;
            co_await boost::asio::async_read(s, boost::asio::buffer(&hdr, sizeof(hdr)), boost::asio::deferred);

            auto sz = util::nbeswap(hdr);
            data.resize(sz);
            Verbose("receiving {} bytes...", sz);
            co_await boost::asio::async_read(s, boost::asio::buffer(data), boost::asio::deferred);
            
            Info("received [{}]", cxx_coro::binaryToAscii({ data.data(), sz }));

            char* out = nullptr;
            Py_ssize_t len = 0;
            {
                VerboseBlock("GIL locked");

                auto state{ PyGILState_Ensure() };
                Py_XDECREF(appReturn);

                auto pyBytes{ PyBytes_FromStringAndSize(data.data(), data.size()) };
                if (!pyBytes)
                {
                    PyGILState_Release(state);
                    break;
                }

                Verbose("Calling out...");
                appReturn = vecCall ? vecCall(app, &pyBytes, 1, NULL) : PyObject_CallOneArg(app, pyBytes);

                Py_DECREF(pyBytes);

                if (!appReturn) 
                {
                    PyErr_Print();
                    PyErr_Clear();
                    PyGILState_Release(state);
                    break;
                }

                
                if (PyBytes_AsStringAndSize(appReturn, &out, &len)) 
                {
                    PyErr_Print();
                    PyErr_Clear();
                    Py_DECREF(appReturn);
                    PyGILState_Release(state);
                    break;
                }

                PyGILState_Release(state);
            }

            Verbose("sending {} bytes...", len);

            hdr = util::nbeswap(static_cast<std::uint16_t>(len));

            std::array seq{ boost::asio::buffer(&hdr, sizeof(hdr)), boost::asio::buffer(out, len) };
            co_await boost::asio::async_write(s, seq, boost::asio::deferred);
        }
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());

        auto state{ PyGILState_Ensure() };
        Py_XDECREF(appReturn);
        PyGILState_Release(state);
    }
}

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::endpoint ep, PyObject* app) 
{
    VerboseBlock("listener()");

    auto executor{ co_await boost::asio::this_coro::executor };
    boost::asio::ip::tcp::acceptor acceptor{ executor, ep };

    for (;;) 
    {
        Verbose("accepting...");
        boost::asio::ip::tcp::socket socket{ co_await acceptor.async_accept(boost::asio::deferred) };

        Info("new connection started");
        boost::asio::co_spawn(executor, client_handler(std::move(socket), app), boost::asio::detached);
    }
}

void accept(boost::asio::execution::executor auto ex, std::string_view host, std::string_view port, PyObject* app) 
{
    VerboseBlock("accept()");

    boost::asio::ip::tcp::resolver resolver{ ex };

    for (auto re : resolver.resolve(host, port))
    {
        auto ep{ re.endpoint() };

        Info("Listening on: {}:{}", ep.address().to_string(), ep.port());

        boost::asio::co_spawn(ex, listener(std::move(ep), app), boost::asio::detached);
    }
}

PyObject* run(PyObject* self, PyObject* const* args, Py_ssize_t nargs, PyObject* kwnames) 
{
    VerboseBlock("run()");

    static const char* keywords[]{ "app", "host", "port" };
    static _PyArg_Parser parser{ .format = "O|ss:run", .keywords = keywords };

    PyObject* app;
    const char* host = "localhost";
    const char* port = "8000";


    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &parser, &app, &host, &port))
        return nullptr;

    Py_IncRef(app);

    auto old_sigint{ std::signal(SIGINT, SIG_DFL) };

    boost::asio::io_context io{ 1 };
    boost::asio::signal_set signals{ io, SIGINT };
    signals.async_wait([&](auto, auto) 
    {
        PyErr_SetInterrupt();
        io.stop();
    });

    accept(io.get_executor(), host, port, app);

    PyThreadState* thread{ PyEval_SaveThread() };
    io.run();

    signals.cancel();
    signals.clear();
    std::signal(SIGINT, old_sigint);

    PyEval_RestoreThread(thread);
    Py_DecRef(app);
    Py_RETURN_NONE;
}

static PyMethodDef ThisMethods[]
{
    {"run", (PyCFunction)run, METH_FASTCALL | METH_KEYWORDS},
    {0},
};

static PyModuleDef ThisModule
{
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "Echo",
    .m_doc = "Example of a server that delegates to a Python function",
    .m_size = -1,
    .m_methods = ThisMethods,
};

PyMODINIT_FUNC PyInit_Echo(void) 
{
    VerboseBlock("PyInit_Echo()");

    auto mod{ PyModule_Create(&ThisModule) };
    
    if (!mod || PyModule_AddStringConstant(mod, "__version__", "1.0.0"))
        return nullptr;

    return mod;
}



