@echo off
setlocal enabledelayedexpansion

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
echo Usage: run_cmake ^<x86^|x64^> ^<windows^|linux^>  [static]
echo.
goto Exit



:ParseArgs
if "%~1" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%~1
)

if "%~2" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET%-%~2
	if "%~2" EQU "windows" (
		if "%~1" EQU "x64" (
			set CMAKE_GENERATOR=%CMAKE_GENERATOR% Win64
		)
	)
)

if "%~3" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET%-%~3
)

if "%~3" EQU "static" (
	set BUILD_SHARED_LIBS=OFF
) else (
	set BUILD_SHARED_LIBS=ON
)

goto :eof



:Start
setlocal
set VCPKG_TARGET_TRIPLET=
set BUILD_SHARED_LIBS=
set CMAKE_GENERATOR=Visual Studio 15 2017

call :ParseArgs %*

echo "%VCPKG_TARGET_TRIPLET%"
echo "%BUILD_SHARED_LIBS%"
echo "%CMAKE_GENERATOR%"

if "" == "%VCPKG_TARGET_TRIPLET%" (
	goto ShowSyntax
)

if "" == "%BUILD_SHARED_LIBS%" (
	goto ShowSyntax
)

vcpkg install curl[openssl]:%VCPKG_TARGET_TRIPLET%
vcpkg install cpprestsdk:%VCPKG_TARGET_TRIPLET%
vcpkg install gtest:%VCPKG_TARGET_TRIPLET%

cmake.exe -G "%CMAKE_GENERATOR%" -DCMAKE_TOOLCHAIN_FILE=D:\sourcecode\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% -DBUILD_SHARED_LIBS=%BUILD_SHARED_LIBS% -DCMAKE_INSTALL_PREFIX=D:\Teemo -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build
endlocal
goto :eof
