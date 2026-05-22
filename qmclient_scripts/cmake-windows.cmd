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

if /I "%~1"=="--build" (
	rem 旧的 build 目录可能把 msvc_deps_prefix 写成乱码，先在进入本次 build 前修正一次。
	rem 这样 Ninja 本轮编译就能重新记录头文件依赖，不必等到下一次构建才生效。
	rem 先修复构建目录里的 msvc_deps_prefix，再进入本次 build，避免旧 rules.ninja 让头文件依赖失效。
	py.exe -3 "%~dp0repair_ninja_msvc_prefix.py" --prepare-build %* >nul 2>&1
	if errorlevel 1 (
		rem 某些环境没有 py launcher，回退到 python 可执行文件。
		python "%~dp0repair_ninja_msvc_prefix.py" --prepare-build %* >nul 2>&1
		if errorlevel 1 exit /b 1
	)
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

set "RULES_FIXED="
rem build/configure 过程中如果触发了重新生成，rules.ninja 可能又被写回坏前缀，所以这里再补一次。
rem configure/build 结束后再补一次，处理本次命令内部重新生成了 rules.ninja 的情况。
py.exe -3 "%~dp0repair_ninja_msvc_prefix.py" %*
if not errorlevel 1 (
	set "RULES_FIXED=1"
) else (
	rem 同样保留 python 回退，避免只因为 py launcher 缺失就跳过修复。
	python "%~dp0repair_ninja_msvc_prefix.py" %*
	if not errorlevel 1 (
		set "RULES_FIXED=1"
	)
)

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
