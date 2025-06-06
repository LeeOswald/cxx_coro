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
#define Py_BUILD_CORE
#include <internal/pycore_modsupport.h>


extern "C" PyAPI_FUNC(int) _PyArg_ParseStackAndKeywords(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames,
    struct _PyArg_Parser *,
    ...);


struct PyResult 
{
    PyObject* obj;
    char* out;
    Py_ssize_t len;
};


template <boost::asio::execution::executor Ex>
struct PythonApp 
{
    PythonApp(Ex ex, PyObject* app)
        : ex_{ std::move(ex) }
        , app_{ app }
        , vecCall_{ PyVectorcall_Function(app) } 
    {
        VerboseBlock("PythonApp::PythonApp()");
    };

    template <typename _CompletionToken>
    auto run(const std::vector<char>& data, PyObject* prev, _CompletionToken&& token) 
    {
        VerboseBlock("PythonApp::run()");

        auto init = [this, &data, prev](auto handler) 
        {
            VerboseBlock("PythonApp::run::init()");

            // dispatch to the ex_ executor
            boost::asio::dispatch(ex_, [this, prev, &data, h = std::move(handler)]() mutable 
            {
                VerboseBlock("PythonApp::run::init::2()");

                // asio::append is somewhat analogous to std::bind_back, but for completion tokens.
                // It produces a completion token which passes additional arguments to an underlying completion token.
                // In this instance, the underlying completion token is the completion handler for the entire PythonApp.run asynchronous operation, 
                // and the argument we want it to be invoked with is the result of the Python operation.

                // The completion handler has an associated executor, the executor it originated from.The second dispatch 
                // schedules the completion handler to be run on that executor, and execution inside the original coroutine is resumed by that completion handler.
                boost::asio::dispatch(boost::asio::append(std::move(h), run_impl(data, prev)));
            });
        };

        return boost::asio::async_initiate<_CompletionToken, void(PyResult)>(init, token);
    }

    template <typename _CompletionToken> 
    auto decref_pyobj(PyObject* obj, _CompletionToken&& token) 
    {
        VerboseBlock("PythonApp::decref_pyobj()");

        auto init = [this, obj](auto handler) 
        {
            VerboseBlock("PythonApp::decref_pyobj::init()");
            
            
            boost::asio::dispatch(ex_, [this, obj, h = std::move(handler)]() mutable
            {
                VerboseBlock("PythonApp::decref_pyobj::init::2()");

                decref_pyobj_impl(obj);
                boost::asio::dispatch(std::move(h));
            });
        };

        return boost::asio::async_initiate<_CompletionToken, void()>(init, token);
    }

private:
    PyResult run_impl(const std::vector<char>& data, PyObject* prev) 
    {
        VerboseBlock("PythonApp::run_impl()");

        Py_XDECREF(prev);
        PyResult ret{};

        auto pyBytes{ PyBytes_FromStringAndSize(data.data(), data.size()) };
        if (!pyBytes)
            return ret;

        ret.obj = vecCall_ ? vecCall_(app_, &pyBytes, 1, NULL) : PyObject_CallOneArg(app_, pyBytes);
        Py_DECREF(pyBytes);

        if (!ret.obj) 
        {
            PyErr_Print();
            PyErr_Clear();
            return ret;
        }

        if (PyBytes_AsStringAndSize(ret.obj, &ret.out, &ret.len)) 
        {
            PyErr_Print();
            PyErr_Clear();
            Py_CLEAR(ret.obj);
        }

        return ret;
    }

    void decref_pyobj_impl(PyObject* obj) 
    {
        VerboseBlock("PythonApp::decref_pyobj_impl()");

        Py_DECREF(obj);
    }

    Ex ex_;
    PyObject* app_;
    vectorcallfunc vecCall_;
};

namespace util
{

constexpr auto nbeswap(std::integral auto val) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
        return val;

    return std::byteswap(val);
}

} // namespace util {}


boost::asio::awaitable<void> client_handler(boost::asio::ip::tcp::socket s, auto& app)
{
    VerboseBlock("client_handler()");

    PyResult pr{};

    try 
    {
        std::vector<char> data;

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

            pr = co_await app.run(data, pr.obj, boost::asio::deferred);
            if (!pr.obj)
                break;

            Verbose("sending {} bytes...", pr.len);

            hdr = util::nbeswap(static_cast<std::uint16_t>(pr.len));

            std::array seq{ boost::asio::buffer(&hdr, sizeof(hdr)), boost::asio::buffer(pr.out, pr.len) };
            co_await boost::asio::async_write(s, seq, boost::asio::deferred);
        }
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());
    }

    if (pr.obj)
        co_await app.decref_pyobj(pr.obj, boost::asio::deferred);
}

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::endpoint ep, auto& app)
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

void accept(boost::asio::execution::executor auto ex, std::string_view host, std::string_view port, auto& app)
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

    PyObject* appObj;
    const char* host = "localhost";
    const char* port = "8000";


    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &parser, &appObj, &host, &port))
        return nullptr;

    Py_IncRef(appObj);

    auto old_sigint{ std::signal(SIGINT, SIG_DFL) };

    boost::asio::io_context io{ 1 };
    boost::asio::signal_set signals{ io, SIGINT };
    signals.async_wait([&](auto, auto) 
    {
        PyErr_SetInterrupt();
        io.stop();
    });

    PythonApp app{ boost::asio::make_strand(io), appObj };
    accept(io.get_executor(), host, port, app);

    io.run();

    signals.cancel();
    signals.clear();
    std::signal(SIGINT, old_sigint);

    Py_DecRef(appObj);
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



