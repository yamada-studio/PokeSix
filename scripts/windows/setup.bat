@echo off
rem [Windows] 외부 의존성 한 번에 설치 — winget + aqtinstall.
rem
rem   scripts\windows\setup.bat              VS 2022 C++ / CMake / Git / uv + Qt
rem   scripts\windows\setup.bat --no-system  Qt 만
rem   scripts\windows\setup.bat --no-qt      도구만
rem   scripts\windows\setup.bat --yes        확인 질문 없이 (VS 설치 포함)
rem
rem 이미 설치된 것은 건너뛴다. 여러 번 실행해도 안전하다.
rem Visual Studio 설치는 수 GB 라서 --yes 가 아니면 먼저 묻는다.
setlocal
call "%~dp0env.bat" || exit /b 1

set "DO_SYSTEM=1"
set "DO_QT=1"
set "ASSUME_YES=0"

:parse
if "%~1"=="" goto :parsed
if /i "%~1"=="--no-system" (set "DO_SYSTEM=0" & goto :next)
if /i "%~1"=="--no-qt"     (set "DO_QT=0" & goto :next)
if /i "%~1"=="--yes"       (set "ASSUME_YES=1" & goto :next)
if /i "%~1"=="-h"          goto :usage
if /i "%~1"=="--help"      goto :usage
echo error: unknown argument: %~1 1>&2
goto :usage_fail
:next
shift
goto :parse
:parsed


rem ── 1. 도구 (winget) ─────────────────────────────────────────
if "%DO_SYSTEM%"=="0" goto :qt

where winget >nul 2>&1
if errorlevel 1 goto :no_winget

rem Visual Studio 2022 + C++ 데스크톱 워크로드
call "%~dp0env.bat" :find_vs
if not errorlevel 1 goto :vs_found
echo [setup] Visual Studio 2022 with C++ tools: not found
if "%ASSUME_YES%"=="1" goto :vs_install
choice /M "Install Visual Studio 2022 Community with the C++ desktop workload - several GB"
if errorlevel 2 goto :vs_skipped
:vs_install
echo [setup] installing Visual Studio 2022 Community ...
winget install -e --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --accept-source-agreements --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive --wait"
if errorlevel 1 goto :fail
call "%~dp0env.bat" :find_vs
if errorlevel 1 goto :fail
:vs_found
echo [setup] Visual Studio 2022: %VS_INSTALL_DIR%
goto :vs_done
:vs_skipped
echo warn: skipped Visual Studio. build.bat will fail until it is installed. 1>&2
:vs_done

rem CMake — VS 2022 에 번들된 것이 있으면 그것을 쓴다
call "%~dp0env.bat" :find_cmake
if not errorlevel 1 goto :cmake_done
call :winget_install Kitware.CMake
if errorlevel 1 goto :fail
:cmake_done

call :ensure_tool git Git.Git
if errorlevel 1 goto :fail
call :ensure_tool uvx astral-sh.uv
if errorlevel 1 goto :fail


rem ── 2. Qt (aqtinstall) ───────────────────────────────────────
:qt
if "%DO_QT%"=="0" goto :summary
if not defined QT_ROOT_DIR goto :qt_default
if exist "%QT_ROOT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" goto :qt_env
:qt_default
if exist "%QT_DEFAULT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" goto :qt_already

where uvx >nul 2>&1
if errorlevel 1 goto :no_uv
echo [setup] installing Qt %QT_VERSION% to %QT_INSTALL_DIR% ...
rem aqt 는 실행 위치에 aqtinstall.log 를 남기므로 임시 폴더에서 실행한다
pushd "%TEMP%"
uvx --from aqtinstall aqt install-qt windows desktop %QT_VERSION% %QT_AQT_ARCH% --outputdir "%QT_INSTALL_DIR%"
set "AQT_RC=%errorlevel%"
popd
if not "%AQT_RC%"=="0" goto :fail
if not exist "%QT_DEFAULT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" goto :fail
goto :summary

:qt_env
echo [setup] Qt: using QT_ROOT_DIR=%QT_ROOT_DIR%
goto :summary
:qt_already
echo [setup] Qt %QT_VERSION%: already installed at %QT_DEFAULT_DIR%
goto :summary


rem ── 3. 확인 ──────────────────────────────────────────────────
:summary
echo.
echo [setup] check
call "%~dp0env.bat" :find_cmake
if errorlevel 1 (echo warn: cmake not found 1>&2) else (cmake --version | findstr /b "cmake")
if "%DO_QT%"=="0" goto :next_steps
call "%~dp0env.bat" :resolve_qt
if errorlevel 1 goto :fail
echo Qt: %QT_ROOT_DIR%

:next_steps
echo.
echo Next:
echo   scripts\windows\build.bat      configure + build + test
echo   scripts\windows\run.bat        run
echo.
echo scripts\windows\*.bat find Qt at %QT_DEFAULT_DIR% even without QT_ROOT_DIR.
echo To use cmake --preset directly or build from an IDE, set it once and reopen the terminal:
echo   setx QT_ROOT_DIR "%QT_DEFAULT_DIR%"
exit /b 0


rem ── 서브루틴 ─────────────────────────────────────────────────

rem :ensure_tool <exe> <winget-id> — exe 가 PATH 에 없으면 winget 으로 설치
:ensure_tool
where %1 >nul 2>&1
if errorlevel 1 goto :ensure_tool_install
echo [setup] %1: found
exit /b 0
:ensure_tool_install
call :winget_install %2
exit /b %errorlevel%

rem :winget_install <winget-id> — 설치 후 이 세션의 PATH 도 갱신한다
:winget_install
echo [setup] installing %1 ...
winget install -e --id %1 --accept-package-agreements --accept-source-agreements
if errorlevel 1 exit /b 1
rem winget 이 등록한 PATH 는 새 터미널에서만 보이므로 알려진 설치 위치를 직접 추가한다
set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Links;%USERPROFILE%\.local\bin;%ProgramFiles%\Git\cmd;%ProgramFiles%\CMake\bin;%PATH%"
exit /b 0


:no_winget
echo error: winget not found. Install "App Installer" from the Microsoft Store, then retry. 1>&2
exit /b 1

:no_uv
echo error: uv not found. Run without --no-system, or install it: winget install astral-sh.uv 1>&2
exit /b 1

:fail
echo error: setup failed. 1>&2
exit /b 1

:usage
echo usage: scripts\windows\setup.bat [--no-system] [--no-qt] [--yes]
exit /b 0

:usage_fail
echo usage: scripts\windows\setup.bat [--no-system] [--no-qt] [--yes]
exit /b 1
