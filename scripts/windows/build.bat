@echo off
rem [Windows] configure -> build -> test (-> install) in one go. MSVC 2022, Visual Studio generator.
rem
rem   scripts\windows\build.bat                     debug: configure + build + test
rem   scripts\windows\build.bat release             release
rem   scripts\windows\build.bat --clean             delete the build directory and start over
rem   scripts\windows\build.bat --no-test           skip tests
rem   scripts\windows\build.bat release --install C:\Apps\PokeSix
rem
rem Build directory: build\windows-msvc (multi-config — Debug and Release share one directory)
rem No Developer Command Prompt needed: CMake finds VS 2022 itself via vswhere.
setlocal
call "%~dp0env.bat" || exit /b 1

set "CONFIG=debug"
set "CLEAN=0"
set "RUN_TESTS=1"
set "INSTALL_PREFIX="

:parse
if "%~1"=="" goto :parsed
if /i "%~1"=="debug"     (set "CONFIG=debug" & goto :next)
if /i "%~1"=="release"   (set "CONFIG=release" & goto :next)
if /i "%~1"=="--clean"   (set "CLEAN=1" & goto :next)
if /i "%~1"=="--no-test" (set "RUN_TESTS=0" & goto :next)
if /i "%~1"=="--install" goto :opt_install
if /i "%~1"=="-h"        goto :usage
if /i "%~1"=="--help"    goto :usage
echo error: unknown argument: %~1 1>&2
goto :usage_fail
:opt_install
if "%~2"=="" goto :install_needs_path
set "INSTALL_PREFIX=%~2"
shift /1
:next
shift /1
goto :parse
:parsed

if /i "%CONFIG%"=="release" (set "CFG=Release") else (set "CFG=Debug")
set "BUILD_DIR=%POKESIX_ROOT%\build\windows-msvc"

call "%~dp0env.bat" :resolve_qt
if errorlevel 1 exit /b 1
call "%~dp0env.bat" :find_cmake
if errorlevel 1 goto :no_cmake

cd /d "%POKESIX_ROOT%"
echo [build] config=%CFG%  Qt=%QT_ROOT_DIR%

if not "%CLEAN%"=="1" goto :configure
if not exist "%BUILD_DIR%" goto :configure
echo [build] clean: %BUILD_DIR%
rmdir /s /q "%BUILD_DIR%"

:configure
echo [build] configure
cmake --preset windows-msvc
if errorlevel 1 goto :fail

echo [build] build
cmake --build --preset windows-%CONFIG%
if errorlevel 1 goto :fail

if not "%RUN_TESTS%"=="1" goto :install
echo [build] test
ctest --preset windows-%CONFIG%
if errorlevel 1 goto :fail

:install
if not defined INSTALL_PREFIX goto :done
echo [build] install: %INSTALL_PREFIX%
cmake --install "%BUILD_DIR%" --config %CFG% --prefix "%INSTALL_PREFIX%"
if errorlevel 1 goto :fail

:done
echo [build] done: %BUILD_DIR%\src\%CFG%\PokeSix.exe
exit /b 0


:no_cmake
echo error: cmake not found. Run scripts\windows\setup.bat 1>&2
exit /b 1

:install_needs_path
echo error: --install needs a path 1>&2
exit /b 1

:fail
echo error: build failed. 1>&2
exit /b 1

:usage
echo usage: scripts\windows\build.bat [debug^|release] [--clean] [--no-test] [--install PREFIX]
exit /b 0

:usage_fail
echo usage: scripts\windows\build.bat [debug^|release] [--clean] [--no-test] [--install PREFIX]
exit /b 1
