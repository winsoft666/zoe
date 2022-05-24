@echo off

cmake.exe -G "Visual Studio 16 2019" -T "v142" -A "Win32" -DCMAKE_TOOLCHAIN_FILE=D:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=D:\Teemo -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build

pause
