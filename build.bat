@echo off

goto Start

:ECHORED
%Windir%\System32\WindowsPowerShell\v1.0\Powershell.exe write-host -foregroundcolor Red %1
goto :eof



:Exit
endlocal
exit /B 1



:ShowError
echo.
call :ECHORED %1
goto :eof



:ShowSyntax
rem Display the help
echo.
echo Usage: build ^<build_type^>
echo.
echo build_type:
echo.
echo static        - Use Build Static Library
echo shared        - Use Build Shared Library
echo.
goto Exit



:ParseArgs

if "%~1" == "static" (
	set VCPKG_TARGET_TRIPLET=x86-windows-static
	set BUILD_SHARED_LIBS=OFF
) else if "%~1" == "shared" (
	set VCPKG_TARGET_TRIPLET=x86-windows
	set BUILD_SHARED_LIBS=ON
)

shift

if NOT "%~1" == "" (
	goto ParseArgs
)
goto :eof



:Start
setlocal
set VCPKG_TARGET_TRIPLET=
set BUILD_SHARED_LIBS=

call :ParseArgs %*

if "" == "%VCPKG_TARGET_TRIPLET%" (
	goto ShowSyntax
)

if "" == "%BUILD_SHARED_LIBS%" (
	goto ShowSyntax
)

vcpkg install curl[openssl]:%VCPKG_TARGET_TRIPLET%
vcpkg install gtest:%VCPKG_TARGET_TRIPLET%

cmake.exe -G "Visual Studio 15" -DCMAKE_TOOLCHAIN_FILE=D:\sourcecode\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% -DBUILD_SHARED_LIBS=%BUILD_SHARED_LIBS% -DCMAKE_INSTALL_PREFIX=D:\Teemo -DBUILD_TESTS=OFF -S %~dp0 -B %~dp0build

cmake.exe --build %~dp0build --config Debug --target INSTALL
cmake.exe --build %~dp0build --config Release --target INSTALL


endlocal
goto :eof

