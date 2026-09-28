@echo off
rem [Windows] PokeSix 실행. 빌드된 실행 파일이 없으면 먼저 빌드한다.
rem
rem   scripts\windows\run.bat                       debug 빌드 실행
rem   scripts\windows\run.bat release
rem   scripts\windows\run.bat --rebuild             실행 전에 항상 빌드 (테스트 생략)
rem   scripts\windows\run.bat --log                 pokesix.* debug 로그 켜기
rem   scripts\windows\run.bat --vs                  Visual Studio 디버거에서 실행
rem   scripts\windows\run.bat --offscreen           창 없이 실행 (CI, 캡처)
rem   scripts\windows\run.bat -- --screenshot home 1440x900 out.png     -- 뒤는 앱 인자
rem
rem 로그: PokeSix 는 GUI(WIN32) 앱이라 콘솔이 없다. 그래서 stderr 를 파일로 받아
rem       (build\windows-msvc\PokeSix-<config>.log) 앱이 끝난 뒤 출력한다.
rem       실행 중에 보려면 --vs 로 띄우고 Visual Studio 출력 창을 본다.
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

rem -- 뒤의 인자는 그대로 앱에 넘긴다
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
rem Windows 에는 RPATH 가 없다. Qt DLL 과 플러그인을 찾도록 Qt bin 을 PATH 앞에 둔다
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
