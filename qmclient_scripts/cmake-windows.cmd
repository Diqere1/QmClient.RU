@echo off
setlocal

if "%~1"=="" goto usage

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDEVCMD="

if exist "%VSWHERE%" (
	for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do (
		set "VSDEVCMD=%%I"
	)
)

if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"

if not defined VSDEVCMD (
	echo Failed to locate VsDevCmd.bat. Install Visual Studio 2022 with MSVC build tools.
	exit /b 1
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%

if defined VSINSTALLDIR (
	if exist "%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" set "PATH=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
)

set "CMOUT=%TEMP%\cmake-windows-%RANDOM%.log"
if /I "%~1"=="--build" (
	cmake %* > "%CMOUT%" 2>&1
) else if /I "%~1"=="-E" (
	cmake %* > "%CMOUT%" 2>&1
) else if /I "%~1"=="-P" (
	cmake %* > "%CMOUT%" 2>&1
) else if /I "%~1"=="--install" (
	cmake %* > "%CMOUT%" 2>&1
) else if /I "%~1"=="--open" (
	cmake %* > "%CMOUT%" 2>&1
) else if /I "%~1"=="--workflow" (
	cmake %* > "%CMOUT%" 2>&1
) else (
	cmake -Wno-dev %* > "%CMOUT%" 2>&1
)
set "CMRC=%errorlevel%"
set "FILTERED="
py.exe -3 "%~dp0cmake-windows-filter.py" "%CMOUT%"
if not errorlevel 1 (
	set "FILTERED=1"
) else (
	python "%~dp0cmake-windows-filter.py" "%CMOUT%"
	if not errorlevel 1 (
		set "FILTERED=1"
	)
)
if not defined FILTERED type "%CMOUT%"
del /Q "%CMOUT%" >nul 2>&1
exit /b %CMRC%

:usage
echo Usage: scripts\cmake-windows.cmd [cmake arguments]
echo Example: scripts\cmake-windows.cmd -S . -B cmake-build-release
echo Example: scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10
exit /b 1
