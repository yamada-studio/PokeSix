@echo off
rem [Windows] Install all external dependencies in one go — winget + aqtinstall.
rem
rem   scripts\windows\setup.bat              VS 2022 C++ / CMake / Git / uv + Qt
rem   scripts\windows\setup.bat --no-system  Qt only
rem   scripts\windows\setup.bat --no-qt      tools only
rem   scripts\windows\setup.bat --yes        no prompts (includes installing Visual Studio)
rem
rem Anything already installed is skipped. Safe to run repeatedly.
rem Visual Studio is several GB, so it asks first unless --yes is given.
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
shift /1
goto :parse
:parsed


rem ── 1. Tools (winget) ───────────────────────────────────────
if "%DO_SYSTEM%"=="0" goto :qt

where winget >nul 2>&1
if errorlevel 1 goto :no_winget

rem Visual Studio 2022 + the C++ desktop workload
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

rem CMake — use the one bundled with VS 2022 if present
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
echo     (aqt prints a line only when an archive finishes. qtbase and qtdeclarative are
echo      hundreds of MB, so it can stay silent for 10+ minutes while they download and
echo      extract - it is not stuck.)
rem aqt leaves aqtinstall.log in the working directory, so run it from a temp directory
pushd "%TEMP%"
rem --archives: only what PokeSix uses (qtbase/qtsvg/qttools) - skips qtdeclarative & friends
uvx --from aqtinstall aqt install-qt windows desktop %QT_VERSION% %QT_AQT_ARCH% --archives qtbase qtsvg qttools --outputdir "%QT_INSTALL_DIR%"
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


rem ── 3. Check ────────────────────────────────────────────────
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


rem ── Subroutines ─────────────────────────────────────────────

rem :ensure_tool <exe> <winget-id> — install with winget if exe is not on PATH
:ensure_tool
where %1 >nul 2>&1
if errorlevel 1 goto :ensure_tool_install
echo [setup] %1: found
exit /b 0
:ensure_tool_install
call :winget_install %2
exit /b %errorlevel%

rem :winget_install <winget-id> — also refreshes PATH for this session after installing
:winget_install
echo [setup] installing %1 ...
winget install -e --id %1 --accept-package-agreements --accept-source-agreements
if errorlevel 1 exit /b 1
rem PATH entries registered by winget only appear in new terminals, so add the known install locations here
rem winget 1.29 keeps portable packages (uv) under WinGet\Packages and may not create WinGet\Links (symlinks need Developer Mode)
for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\astral-sh.uv_*") do set "PATH=%%D;%PATH%"
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
