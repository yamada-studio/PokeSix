@echo off
rem [Windows] scripts\windows\*.bat 공통 설정. 직접 실행하지 않고 call 한다.
rem
rem   call "%~dp0env.bat"                변수 설정
rem   call "%~dp0env.bat" :resolve_qt    QT_ROOT_DIR 확정 (실패 시 errorlevel 1)
rem   call "%~dp0env.bat" :find_vs       VS_INSTALL_DIR 설정 (C++ 도구가 있는 VS 2022)
rem   call "%~dp0env.bat" :find_cmake    cmake 를 PATH 에서, 없으면 VS 2022 번들에서 찾아 PATH 에 추가
rem
rem 작성 규칙:
rem   - setlocal 을 쓰지 않는다. 여기서 정한 변수는 호출한 스크립트에 남아야 한다
rem   - ( ) 블록 안에서 경로 변수를 펼치지 않는다. "Program Files (x86)" 의 ) 가 블록을 닫아 버린다
rem   - 화면 출력은 영어로. 한국어 Windows 콘솔(CP949)에서 UTF-8 한글이 깨진다

for %%I in ("%~dp0..\..") do set "POKESIX_ROOT=%%~fI"

rem Qt 버전은 scripts\QT_VERSION 한 곳에서 관리한다 (환경 변수로 덮어쓸 수 있음)
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
rem [17.0,18.0) = VS 2022 만 (프리셋 generator 가 "Visual Studio 17 2022")
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
