@echo off
rem [Windows] Package a release build as a portable ZIP.
rem
rem   scripts\windows\package.bat             release build (with tests) -> ZIP
rem   scripts\windows\package.bat --no-build  reuse the existing release build
rem
rem Output: build\windows-msvc\package\PokeSix-<version>-win64.zip
rem The install step runs windeployqt (qt_generate_deploy_app_script in src/CMakeLists.txt),
rem so the folder carries the Qt DLLs and plugins and runs on a PC without Qt.
setlocal
call "%~dp0env.bat" || exit /b 1

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
exit /b 0

:usage
echo usage: scripts\windows\package.bat [--no-build]
exit /b 0
:usage_fail
echo usage: scripts\windows\package.bat [--no-build]
exit /b 2
