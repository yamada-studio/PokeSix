@echo off
rem [Windows] Package a release build as a portable ZIP.
rem
rem   scripts\windows\package.bat             release build (with tests) -> ZIP
rem   scripts\windows\package.bat --no-build  reuse the existing release build
rem
rem Output: build\windows-msvc\package\PokeSix-<version>-win64.zip   (portable)
rem         build\windows-msvc\package\PokeSix-<version>-win64.msi   (installer, needs WiX 3.14)
rem The install step runs windeployqt (qt_generate_deploy_app_script in src/CMakeLists.txt),
rem so the folder carries the Qt DLLs and plugins and runs on a PC without Qt. The .msi is the
rem same tree wrapped by CPack's WIX generator (start-menu shortcut, upgrade in place); when the
rem WiX Toolset is missing the step is skipped with a hint - GitHub runners have it preinstalled.
setlocal
call "%~dp0env.bat" || exit /b 1
rem cmake --install below runs in this script, not in build.bat (whose PATH change is local to it)
call "%~dp0env.bat" :find_cmake
if errorlevel 1 (
    echo error: cmake not found. Run scripts\windows\setup.bat 1>&2
    exit /b 1
)

set "DO_BUILD=1"
:parse
if "%~1"=="" goto :parsed
if /i "%~1"=="--no-build" (set "DO_BUILD=0" & goto :next)
if /i "%~1"=="-h"         goto :usage
if /i "%~1"=="--help"     goto :usage
echo error: unknown argument: %~1 1>&2
goto :usage_fail
:next
shift /1
goto :parse
:parsed

rem The version lives in one place: project(VERSION x.y.z) in CMakeLists.txt
set "VERSION="
for /f "tokens=2" %%v in ('findstr /r /c:"^ *VERSION [0-9]" "%POKESIX_ROOT%\CMakeLists.txt"') do (
    if not defined VERSION set "VERSION=%%v"
)
if not defined VERSION (
    echo error: cannot read VERSION from CMakeLists.txt 1>&2
    exit /b 1
)

if "%DO_BUILD%"=="1" (
    call "%~dp0build.bat" release
    if errorlevel 1 exit /b 1
)
if not exist "%POKESIX_ROOT%\build\windows-msvc\src\Release\PokeSix.exe" (
    echo error: no release build. Run scripts\windows\build.bat release first. 1>&2
    exit /b 1
)

set "PKG=%POKESIX_ROOT%\build\windows-msvc\package"
if exist "%PKG%\PokeSix" rmdir /s /q "%PKG%\PokeSix"
rem rmdir does not fail when a file inside is in use (Windows keeps open files), so check the result:
rem a PokeSix.exe started from this folder (quickstart does that) blocks the whole packaging.
if exist "%PKG%\PokeSix" (
    echo error: cannot clear %PKG%\PokeSix - is PokeSix running from that folder? Close it and retry. 1>&2
    exit /b 1
)
echo ==^> install + windeployqt -^> %PKG%\PokeSix
cmake --install "%POKESIX_ROOT%\build\windows-msvc" --config Release --prefix "%PKG%\PokeSix"
if errorlevel 1 exit /b 1

rem Ship the licenses with the binaries (MIT + Qt LGPL / font notices)
copy /y "%POKESIX_ROOT%\LICENSE" "%PKG%\PokeSix\" >nul
copy /y "%POKESIX_ROOT%\THIRD_PARTY_NOTICES.md" "%PKG%\PokeSix\" >nul

set "ZIP=%PKG%\PokeSix-%VERSION%-win64.zip"
if exist "%ZIP%" del "%ZIP%"
echo ==^> zip -^> %ZIP%
powershell -NoProfile -Command "Compress-Archive -Path '%PKG%\PokeSix' -DestinationPath '%ZIP%'"
if errorlevel 1 exit /b 1

echo done: %ZIP%
powershell -NoProfile -Command "(Get-FileHash '%ZIP%' -Algorithm SHA256).Hash"

rem --- MSI installer (CPack WIX generator; cpack.exe sits next to cmake.exe) ---------------
rem CPack finds WiX through the WIX environment variable or candle.exe on PATH.
set "HAVE_WIX="
if defined WIX set "HAVE_WIX=1"
if not defined HAVE_WIX (
    where candle.exe >nul 2>&1
    if not errorlevel 1 set "HAVE_WIX=1"
)
if not defined HAVE_WIX (
    echo note: WiX Toolset not found - skipping the .msi installer. Install it with:
    echo   winget install --id WiXToolset.WiXToolset
    exit /b 0
)
set "MSI=%PKG%\PokeSix-%VERSION%-win64.msi"
if exist "%MSI%" del "%MSI%"
echo ==^> msi -^> %MSI%
pushd "%POKESIX_ROOT%\build\windows-msvc"
cpack -G WIX -C Release -B "%PKG%"
if errorlevel 1 (popd & exit /b 1)
popd
if not exist "%MSI%" (
    echo error: cpack finished but %MSI% is missing 1>&2
    exit /b 1
)
echo done: %MSI%
powershell -NoProfile -Command "(Get-FileHash '%MSI%' -Algorithm SHA256).Hash"
exit /b 0

:usage
echo usage: scripts\windows\package.bat [--no-build]
exit /b 0
:usage_fail
echo usage: scripts\windows\package.bat [--no-build]
exit /b 2
