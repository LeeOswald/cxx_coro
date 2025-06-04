# cxx_coro
Experiments with C++'20 coroutines

## what's in
```
generator                          simple coroutine-based generator
interruptible                      cancellable coroutines
echo_server                        TCP echo server
echo_client.py                     client for echo server testing 
py_echo_server                     echo-server as a native module (Echo.pyd) for echo_server.py (see below)
echo_server.py                     python echo server (needs Echo.pyd in $PATH)
```

## building
The project depends on *Boost* and *{fmt}*. You can either install them manually or use **Conan 2**. Use *update_conan.cmd* as a reference of just run it.
After installing the dependencies, build the project just like you would build a usual CMake-based project. You may use *generate_windows.cmd* as a reference.

