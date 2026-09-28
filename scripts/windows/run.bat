@echo off
rem [Windows] Run PokeSix. Builds first if the executable does not exist yet.
rem
rem   scripts\windows\run.bat                       run the debug build
rem   scripts\windows\run.bat release
rem   scripts\windows\run.bat --rebuild             always build before running (tests skipped)
rem   scripts\windows\run.bat --log                 enable pokesix.* debug logs
rem   scripts\windows\run.bat --vs                  run under the Visual Studio debugger
rem   scripts\windows\run.bat --offscreen           run without a window (CI, screenshots)
rem   scripts\windows\run.bat -- --screenshot home 1440x900 out.png     everything after -- goes to the app
rem
rem Logs: PokeSix is a GUI (WIN32) app and has no console, so stderr is captured to a file
rem       (build\windows-msvc\PokeSix-<config>.log) and printed after the app exits.
rem       To watch logs live, launch with --vs and use the Visual Studio Output window.
setlocal
call "%~dp0env.bat" || exit /b 1

set "CONFIG=debug"
set "REBUILD=0"
set "MODE=direct"
set "APP_ARGS="

:parse
if "%~1"=="" goto :parsed
if "%~1"=="--" goto :collect
if /i "%~1"=="debug"       (set "CONFIG=debug" & goto :next)
if /i "%~1"=="release"     (set "CONFIG=release" & goto :next)
if /i "%~1"=="--rebuild"   (set "REBUILD=1" & goto :next)
if /i "%~1"=="--log"       (set "QT_LOGGING_RULES=pokesix.*.debug=true" & goto :next)
if /i "%~1"=="--vs"        (set "MODE=vs" & goto :next)
if /i "%~1"=="--offscreen" (set "QT_QPA_PLATFORM=offscreen" & goto :next)
if /i "%~1"=="-h"          goto :usage
if /i "%~1"=="--help"      goto :usage
echo error: unknown argument: %~1 (app arguments go after --) 1>&2
goto :usage_fail
:next
shift
goto :parse

rem Arguments after -- are passed to the app unchanged
:collect
shift
:collect_loop
if "%~1"=="" goto :parsed
set APP_ARGS=%APP_ARGS% %1
shift
goto :collect_loop
:parsed

if /i "%CONFIG%"=="release" (set "CFG=Release") else (set "CFG=Debug")
set "BUILD_DIR=%POKESIX_ROOT%\build\windows-msvc"
set "BIN=%BUILD_DIR%\src\%CFG%\PokeSix.exe"
set "LOG=%BUILD_DIR%\PokeSix-%CONFIG%.log"

if "%REBUILD%"=="1" goto :build
if not exist "%BIN%" goto :build
goto :run
:build
call "%~dp0build.bat" %CONFIG% --no-test
if errorlevel 1 exit /b 1
if not exist "%BIN%" goto :no_binary

:run
call "%~dp0env.bat" :resolve_qt
if errorlevel 1 exit /b 1
rem Windows has no RPATH: put Qt bin first on PATH so Qt DLLs and plugins are found
set "PATH=%QT_ROOT_DIR%\bin;%PATH%"

if "%MODE%"=="vs" goto :run_vs

echo [run] %BIN%%APP_ARGS%
set "QT_FORCE_STDERR_LOGGING=1"
"%BIN%"%APP_ARGS% > "%LOG%" 2>&1
set "RC=%errorlevel%"
echo [run] exit code %RC%, log: %LOG%
type "%LOG%"
exit /b %RC%

:run_vs
call "%~dp0env.bat" :find_vs
if errorlevel 1 goto :no_vs
echo [run] Visual Studio debugger: %BIN%%APP_ARGS%
start "" "%VS_INSTALL_DIR%\Common7\IDE\devenv.exe" /debugexe "%BIN%"%APP_ARGS%
exit /b 0


:no_binary
echo error: executable not found: %BIN% 1>&2
exit /b 1

:no_vs
echo error: Visual Studio 2022 not found. Run scripts\windows\setup.bat 1>&2
exit /b 1

:usage
echo usage: scripts\windows\run.bat [debug^|release] [--rebuild] [--log] [--vs] [--offscreen] [-- app-args...]
exit /b 0

:usage_fail
echo usage: scripts\windows\run.bat [debug^|release] [--rebuild] [--log] [--vs] [--offscreen] [-- app-args...]
exit /b 1
