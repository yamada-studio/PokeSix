@echo off
rem [Windows] Everything after a fresh clone in one go:
rem setup (VS + Qt) -> release build + tests -> portable ZIP -> run it.
rem
rem   scripts\windows\quickstart.bat            setup -> package -> run
rem   scripts\windows\quickstart.bat --no-run   stop after packaging
setlocal
call "%~dp0env.bat" || exit /b 1

set "DO_RUN=1"
:parse
if "%~1"=="" goto :parsed
if /i "%~1"=="--no-run" (set "DO_RUN=0" & goto :next)
if /i "%~1"=="-h"       goto :usage
if /i "%~1"=="--help"   goto :usage
echo error: unknown argument: %~1 1>&2
exit /b 2
:next
shift /1
goto :parse
:parsed

call "%~dp0setup.bat"
if errorlevel 1 exit /b 1
call "%~dp0package.bat"
if errorlevel 1 exit /b 1

set "BIN=%POKESIX_ROOT%\build\windows-msvc\package\PokeSix\bin\PokeSix.exe"
if "%DO_RUN%"=="1" (
    echo ==^> run: %BIN%
    start "" "%BIN%"
) else (
    echo ready: %BIN%
)
exit /b 0

:usage
echo usage: scripts\windows\quickstart.bat [--no-run]
exit /b 0
