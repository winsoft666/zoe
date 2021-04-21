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
echo Usage: run_cmake ^<VS_Version^> ^<Toolset^> ^<x86^|x64^> ^<windows^|linux^>  [static]
echo VS_Version: 15, 16
echo Toolset: v141, v142...
echo.
goto Exit



:ParseArgs
if "%~1" NEQ "" (
	if "%~1" EQU "15" (
		set CMAKE_GENERATOR=Visual Studio 15 2017
	) else if "%~1" EQU "16" (
		set CMAKE_GENERATOR=Visual Studio 16 2019
	)
)

if "%~2" NEQ "" (
	set TOOLSET=%~2
)

if "%~3" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%~3
)

if "%~4" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET%-%~4
	if "%~4" EQU "windows" (
		if "%~3" EQU "x64" (
			set ARCH=x64
		) else (
			set ARCH=Win32
		)
	)
)

if "%~5" NEQ "" (
	set VCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET%-%~5
)

if "%~5" EQU "static" (
	set BUILD_SHARED_LIBS=OFF
) else (
	set BUILD_SHARED_LIBS=ON
)

goto :eof



:Start
setlocal
set VCPKG_TARGET_TRIPLET=
set BUILD_SHARED_LIBS=ON
set CMAKE_GENERATOR=Visual Studio 16 2019
set TOOLSET=v141
set ARCH=Win32

call :ParseArgs %*

echo "%CMAKE_GENERATOR%"
echo "%VCPKG_TARGET_TRIPLET%"
echo "%BUILD_SHARED_LIBS%"


if "" == "%VCPKG_TARGET_TRIPLET%" (
	goto ShowSyntax
)

if "" == "%BUILD_SHARED_LIBS%" (
	goto ShowSyntax
)

vcpkg install gtest:%VCPKG_TARGET_TRIPLET%
vcpkg install curl[non-http]:%VCPKG_TARGET_TRIPLET%

cmake.exe -G "%CMAKE_GENERATOR%" -T "%TOOLSET%" -A "%ARCH%" -DCMAKE_TOOLCHAIN_FILE=D:\sourcecode\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% -DBUILD_SHARED_LIBS=%BUILD_SHARED_LIBS% -DCMAKE_INSTALL_PREFIX=D:\Teemo -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build
endlocal
goto :eof
