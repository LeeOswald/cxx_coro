pushd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=generators\conan_toolchain.cmake  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCXX_CORO_VERBOSE_LOG=1
popd
pause


