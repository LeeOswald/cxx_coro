# cxx_coro
Experiments with C++'20 coroutines

## what's in
```
/generator                          simple coroutine-based generator
/interruptible                      cancellable coroutines
```

## building
The project depends on *Boost* and *{fmt}*. You can either install them manually or use **Conan 2**. Use *update_conan.cmd* as a reference of just run it.
After installing the dependencies, build the project just as you would build as a usual CMake-based project. You may use *generate_windows.cmd* as a reference.

