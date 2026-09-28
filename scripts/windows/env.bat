@echo off
rem [Windows] Shared settings for scripts\windows\*.bat. Call it; do not execute it directly.
rem
rem   call "%~dp0env.bat"                set variables
rem   call "%~dp0env.bat" :resolve_qt    resolve QT_ROOT_DIR (errorlevel 1 on failure)
rem   call "%~dp0env.bat" :find_vs       set VS_INSTALL_DIR (VS 2022 with C++ tools)
rem   call "%~dp0env.bat" :find_cmake    find cmake on PATH, else the one bundled with VS 2022 (added to PATH)
rem
rem Rules for this file:
rem   - no setlocal: variables set here must survive in the calling script
rem   - never expand path variables inside ( ) blocks: the ) in "Program Files (x86)" closes the block
rem   - console output in English: UTF-8 Korean text is garbled in a Korean Windows console (CP949)

for %%I in ("%~dp0..\..") do set "POKESIX_ROOT=%%~fI"

rem The Qt version lives in one place, scripts\QT_VERSION (can be overridden by the environment)
if not defined QT_VERSION for /f "usebackq delims=" %%V in ("%POKESIX_ROOT%\scripts\QT_VERSION") do set "QT_VERSION=%%V"
if not defined QT_INSTALL_DIR set "QT_INSTALL_DIR=C:\Qt"
set "QT_AQT_ARCH=win64_msvc2022_64"
set "QT_DEFAULT_DIR=%QT_INSTALL_DIR%\%QT_VERSION%\msvc2022_64"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if "%~1"=="" exit /b 0
goto %~1


:resolve_qt
if defined QT_ROOT_DIR goto :resolve_qt_check
if not exist "%QT_DEFAULT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" goto :resolve_qt_missing
set "QT_ROOT_DIR=%QT_DEFAULT_DIR%"
exit /b 0

:resolve_qt_check
if exist "%QT_ROOT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" exit /b 0
echo error: QT_ROOT_DIR=%QT_ROOT_DIR% does not contain Qt (lib\cmake\Qt6). See docs/build.md 1>&2
exit /b 1

:resolve_qt_missing
echo error: Qt not found at %QT_DEFAULT_DIR%. Run scripts\windows\setup.bat or set QT_ROOT_DIR. 1>&2
exit /b 1


:find_vs
set "VS_INSTALL_DIR="
if not exist "%VSWHERE%" exit /b 1
rem [17.0,18.0) = VS 2022 only (the preset generator is "Visual Studio 17 2022")
for /f "usebackq delims=" %%P in (`call "%VSWHERE%" -version [17.0^,18.0^) -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL_DIR=%%P"
if not defined VS_INSTALL_DIR exit /b 1
exit /b 0


:find_cmake
where cmake >nul 2>&1
if not errorlevel 1 exit /b 0
call :find_vs
if errorlevel 1 exit /b 1
set "VS_CMAKE_BIN=%VS_INSTALL_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
if not exist "%VS_CMAKE_BIN%\cmake.exe" exit /b 1
set "PATH=%VS_CMAKE_BIN%;%PATH%"
exit /b 0
