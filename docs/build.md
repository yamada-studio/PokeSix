# 빌드 가이드

| OS | 컴파일러 | Generator | 프리셋 | 검증 상태 |
|---|---|---|---|---|
| Ubuntu 24.04 | GCC 13+ | Ninja | `linux-debug` / `linux-release` | ✅ 빌드·테스트·실행 확인 (Qt 6.8.3) |
| macOS | Apple Clang (Xcode CLT) | Ninja | `macos-debug` / `macos-release` | ⚠ 미검증 |
| Windows 10/11 | MSVC 2022 | Visual Studio 17 2022 | `windows-msvc` + `windows-debug` / `windows-release` | ✅ 빌드·테스트·실행 확인 (Windows 11, VS 2022 17.14, Qt 6.8.3, 2026-10-04) |

공통 요구 사항: CMake 3.21+, **Qt 6.8 이상** (Widgets / Svg / Sql / Network), 인터넷(첫 configure 때 GoogleTest를 받는다).

---

## 0. 한 번에 — OS별 스크립트

| 단계 | Linux | macOS | Windows |
|---|---|---|---|
| 의존성 설치 (Qt 포함) | `scripts/linux/setup.sh` | `scripts/macos/setup.sh` | `scripts\windows\setup.bat` |
| configure · build · test | `scripts/linux/build.sh` | `scripts/macos/build.sh` | `scripts\windows\build.bat` |
| 실행 | `scripts/linux/run.sh` | `scripts/macos/run.sh` | `scripts\windows\run.bat` |

- 모든 스크립트는 `--help`로 옵션을 보여 준다. 자세한 건 [scripts/README.md](../scripts/README.md)
- 스크립트는 `QT_ROOT_DIR`이 없어도 기본 설치 위치(`~/Qt/<버전>/<arch>`, `C:\Qt\…`)를 찾는다
- 아래 절들은 스크립트가 내부에서 하는 일, 그리고 스크립트 없이 직접 할 때의 방법이다

## 1. 핵심 개념: `QT_ROOT_DIR`

프리셋은 `CMAKE_PREFIX_PATH`를 환경 변수 `QT_ROOT_DIR`에서 받는다. 머신마다 Qt 경로가 다르기 때문이다.
**`QT_ROOT_DIR`은 Qt 버전·컴파일러 폴더**(`bin/`, `lib/`, `plugins/`가 있는 곳)를 가리켜야 한다.

| 설치 방법 | `QT_ROOT_DIR` 예 |
|---|---|
| aqtinstall / 온라인 인스톨러 (Linux) | `~/Qt/6.8.3/gcc_64` |
| aqtinstall / 온라인 인스톨러 (macOS) | `~/Qt/6.8.3/macos` |
| aqtinstall / 온라인 인스톨러 (Windows) | `C:\Qt\6.8.3\msvc2022_64` |
| Homebrew (macOS) | `$(brew --prefix qt)` |

틀리기 쉬운 예: `~/Qt`(상위 폴더), `~/Qt/6.8.3`(컴파일러 폴더가 빠짐), `…/lib/cmake/Qt6`(너무 깊음).

### 셸 밖에서 실행하는 IDE (VS Code, CLion 등)

데스크톱에서 실행한 IDE는 `~/.bashrc`를 읽지 않는다. 그러면 `QT_ROOT_DIR`이 비어서
`Qt6_DIR-NOTFOUND`로 configure가 실패한다. 이럴 때는 git에 올리지 않는 개인 프리셋
`CMakeUserPresets.json`을 만들어 환경 변수를 프리셋 안에서 지정한다.

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "local-debug",
      "inherits": "linux-debug",
      "environment": { "QT_ROOT_DIR": "$env{HOME}/Qt/6.8.3/gcc_64" }
    }
  ]
}
```

프리셋의 `environment`에 정의한 값은 `$env{QT_ROOT_DIR}` 확장보다 먼저 적용된다.

---

## 2. Ubuntu 24.04

### 2-1. 시스템 패키지

```bash
sudo apt install build-essential cmake ninja-build git \
                 libgl1-mesa-dev libxkbcommon-dev libxcb-cursor0 \
                 clang-format clang-tidy
```

- `libgl1-mesa-dev`, `libxkbcommon-dev`: Qt6Gui를 `find_package`할 때 필요하다
- `libxcb-cursor0`: **Qt 6.5부터 X11(xcb) platform plugin이 요구한다.** 없으면 창이 뜨지 않는다([§5-2](#platform-plugin))

### 2-2. Qt 설치 — apt는 쓰지 않는다

Ubuntu 24.04의 `qt6-base-dev`는 **6.4.2**라서 이 프로젝트의 최소 버전(6.8)보다 낮다.

**방법 A — aqtinstall (권장, 계정 불필요)**
```bash
scripts/linux/setup.sh --no-system         # Qt 만 → ~/Qt/6.8.3/gcc_64
echo 'export QT_ROOT_DIR="$HOME/Qt/6.8.3/gcc_64"' >> ~/.bashrc
```
`uvx`가 있으면 그것을, 없으면 `python3 -m venv`를 쓴다(이때는 `python3-venv` 필요 — `setup.sh`가 알아서 설치).

**방법 B — Qt 온라인 인스톨러**
<https://www.qt.io/download-qt-installer>에서 받는다. Qt 계정이 필요하다.
설치할 때 `Qt 6.8.3 → Desktop gcc 64-bit`을 선택한다. 설치 후 `QT_ROOT_DIR`을 위 표대로 설정한다.

### 2-3. 빌드 · 테스트 · 실행

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
./build/linux-debug/src/PokeSix
```

---

## 3. macOS

### 3-1. 도구

```bash
xcode-select --install           # Apple Clang
brew install cmake ninja
```

### 3-2. Qt 설치

**방법 A — aqtinstall (권장, 버전 고정)**
```bash
scripts/macos/setup.sh           # Homebrew 패키지(uv 포함) + Qt → ~/Qt/6.8.3/macos (Intel + Apple Silicon universal)
echo 'export QT_ROOT_DIR="$HOME/Qt/6.8.3/macos"' >> ~/.zshrc
```

**방법 B — Homebrew**
```bash
brew install qt
echo 'export QT_ROOT_DIR="$(brew --prefix qt)"' >> ~/.zshrc
```
Homebrew는 항상 최신 Qt(6.9 이상)를 설치한다. 최소 버전 조건은 만족하지만 LTS 6.8과 **같은 버전은 아니므로**
다른 머신과 동작이 다르면 먼저 버전 차이를 의심한다.

**방법 C — 온라인 인스톨러**: `Qt 6.8.3 → macOS`를 선택한다.

### 3-3. 빌드 · 실행

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug
open build/macos-debug/src/PokeSix.app
```

`.app` 번들로 만들어진다(`MACOSX_BUNDLE`). 터미널 로그를 보려면
`build/macos-debug/src/PokeSix.app/Contents/MacOS/PokeSix`를 직접 실행한다.

---

## 4. Windows 10/11

### 4-1. 도구

한 번에: `scripts\windows\setup.bat` (winget으로 VS 2022 Community + C++ 워크로드 · Git · uv, aqtinstall로 Qt). 직접 할 때:

- **Visual Studio 2022** (Community 가능) — 워크로드 "C++를 사용한 데스크톱 개발"
  - CMake(3.31)가 함께 설치된다. 별도 CMake는 3.21 이상이면 된다. 스크립트는 PATH에 cmake가 없으면 VS에 번들된 것을 `vswhere`로 찾는다
  - 설치에는 **관리자 권한(UAC 창)** 이 필요하다. winget이 설치 관리자를 띄우면 UAC를 승인한다
  - 설치 관리자가 "재시작하면 완료"라고 하지만, 컴파일러 · CMake는 재시작 없이 바로 쓸 수 있었다
- **winget으로 설치한 도구는 새 터미널에서만 PATH에 보인다.** 같은 터미널에서 이어서 쓰려면 터미널을 다시 연다(setup.bat은 세션 안에서 uv 경로를 직접 더해 준다)
- Python은 필요 없다. Qt는 uv(`uvx --from aqtinstall`)가 격리된 환경에서 받는다. Microsoft Store의 python 스텁(`python --version`이 빈 줄)은 무시해도 된다

### 4-2. Qt 설치

**방법 A — aqtinstall (권장)**
```powershell
scripts\windows\setup.bat --no-system      # Qt 만 → C:\Qt\6.8.3\msvc2022_64 (uv 필요)
setx QT_ROOT_DIR "C:\Qt\6.8.3\msvc2022_64"
```
`setx` 후에는 **새 터미널이나 IDE를 다시 열어야** 적용된다.

**방법 B — 온라인 인스톨러**
- 컴포넌트는 `Qt 6.8.3 → MSVC 2022 64-bit`을 고른다. MinGW 키트를 고르면 MSVC 프리셋과 맞지 않는다.
- 설치 경로는 기본값 `C:\Qt`를 쓴다. **공백이나 한글이 들어간 경로(예: `C:\Users\홍길동\…`, `Program Files`)는 피한다.**
  일부 도구가 경로 인용을 제대로 처리하지 못한다.

### 4-3. 빌드 · 테스트 · 실행

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-debug
ctest --preset windows-debug
```

실행하려면 Qt DLL이 `PATH`에 있어야 한다(아래 [§5-5](#windows-dll)).
```powershell
$env:PATH = "$env:QT_ROOT_DIR\bin;$env:PATH"
.\build\windows-msvc\src\Debug\PokeSix.exe
```

---

## 5. 자주 나오는 에러

<a id="qt-not-found"></a>
### 5-1. `Could not find a package configuration file provided by "Qt6"` / 버전 부족

```
Could not find a configuration file for package "Qt6" that is compatible with requested version "6.8".
  /usr/lib/x86_64-linux-gnu/cmake/Qt6/Qt6Config.cmake, version: 6.4.2
```
- `QT_ROOT_DIR`이 비었거나 틀린 경우 → 시스템 Qt(apt 6.4)가 잡힌다. `echo $QT_ROOT_DIR`로 확인한다([§1](#1-핵심-개념-qt_root_dir))
- IDE에서만 실패하면 IDE가 환경 변수를 모르는 것이다 → `CMakeUserPresets.json`
- 경로를 고친 뒤에도 같으면 이전 캐시가 남은 것이다 → `rm -rf build/<preset>`

<a id="platform-plugin"></a>
### 5-2. Qt platform plugin을 찾지/로드하지 못함

이 리포를 처음 세팅할 때 실제로 만난 출력(Ubuntu 24.04, X11):
```
qt.qpa.plugin: From 6.5.0, xcb-cursor0 or libxcb-cursor0 is needed to load the Qt xcb platform plugin.
qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "" even though it was found.
This application failed to start because no Qt platform plugin could be initialized.
```
- **Linux**: `sudo apt install libxcb-cursor0`. 다른 라이브러리가 빠졌으면 확인한다.
  ```bash
  ldd $QT_ROOT_DIR/plugins/platforms/libqxcb.so | grep "not found"
  ```
- 원인을 모를 때: `QT_DEBUG_PLUGINS=1 ./PokeSix`로 플러그인 탐색 과정을 자세히 출력한다
- **Windows**: `platforms/qwindows.dll`을 못 찾는 경우다. `%QT_ROOT_DIR%\bin`이 `PATH`에 있는지 확인한다.
  배포용이면 `windeployqt`
- 디스플레이 없는 환경(CI, SSH): `QT_QPA_PLATFORM=offscreen`

<a id="sqldrivers"></a>
### 5-3. `QSqlDatabase: QSQLITE driver not loaded`

- Qt가 `plugins/sqldrivers/`에서 `qsqlite` 플러그인을 찾지 못한 것이다
- 확인: `ctest --preset <preset> -R env` → `pokesix_env_check`가 사용 가능한 드라이버 목록을 출력한다
- aqtinstall·온라인 인스톨러 Qt에는 기본으로 들어 있다. 없으면 Qt 설치가 불완전한 것이다
- 배포본에서만 실패하면 `sqldrivers/` 폴더를 함께 복사하지 않은 것이다(`windeployqt` / `macdeployqt`가 처리)

<a id="tls"></a>
### 5-4. TLS 백엔드 없음 — HTTPS 요청 실패

```
qt.network.ssl: No functional TLS backend was found
```
Qt 6의 TLS는 플러그인(`plugins/tls/`)으로 동작한다.

| OS | 기본 백엔드 | 필요한 것 |
|---|---|---|
| Linux | `openssl` | 시스템 OpenSSL 3 (`libssl3`, Ubuntu 기본 설치) |
| Windows | `schannel` | 없음 (OS 내장) |
| macOS | `securetransport` | 없음 (OS 내장) |

- 확인: `ctest -R env`의 출력 중 `TLS backends:` 줄
- Linux에서 openssl 백엔드만 있고 실패하면 `ldconfig -p | grep libssl.so.3`으로 OpenSSL을 확인한다

<a id="windows-dll"></a>
### 5-5. Windows: `Qt6Core.dll을 찾을 수 없습니다` / ctest `Exit code 0xc0000135`

- Windows는 RPATH가 없어서 DLL을 `PATH`에서 찾는다
- ctest 프리셋(`windows-debug`)은 `PATH`에 `%QT_ROOT_DIR%\bin`을 자동으로 추가한다
- 직접 실행할 때는 [§4-3](#4-3-빌드--테스트--실행)처럼 `PATH`를 설정한다
- 같은 코드가 **빌드 중에** 나오면 테스트 목록 읽기 단계다 → [§5-7](#gtest-discovery)

<a id="generator-mismatch"></a>
### 5-6. `Error: generator : Ninja  Does not match the generator used previously`

- 같은 빌드 폴더를 다른 generator로 configure한 경우다 → `rm -rf build/<preset>`

<a id="gtest-discovery"></a>
### 5-7. Windows: 빌드 중 `GoogleTestAddTests.cmake … Error running test executable … Exit code 0xc0000135`

```
CMake Error at .../GoogleTestAddTests.cmake:132 (message):
  Error running test executable.
    Path: '.../tests/data/Debug/pokesix_data_tests.exe'
    Result: Exit code 0xc0000135
```
- `gtest_discover_tests()`가 **빌드 직후** 테스트 실행 파일을 돌려 `TEST()` 목록을 읽는데, 그 시점의 PATH에 Qt DLL이 없어 실행이 안 된 것이다([§5-5](#windows-dll)와 같은 원인, 다른 시점)
- 이 리포는 Qt에 링크하는 테스트에 `DISCOVERY_MODE PRE_TEST`를 줘서 목록 읽기를 **ctest 실행 시점**(테스트 프리셋이 PATH에 Qt bin을 넣는다)으로 미뤘다. Qt에 링크하는 테스트 타깃을 새로 만들면 같은 옵션을 준다
- core 테스트(`pokesix_core_tests`)는 Qt를 쓰지 않으므로 그대로 둔다

---

## 6. IDE

### VS Code (현재 개발 환경)

- 확장: **CMake Tools**, **C/C++**(cpptools). 디버거는 gdb(Linux) / lldb(macOS) / MSVC(Windows)
- 명령 팔레트 → `CMake: Select Configure Preset` / `Select Build Preset` / `Select Test Preset`
- 빌드 F7, 실행 Shift+F5, 디버그 Ctrl+F5. `launch.json` 없이 CMake Tools가 디버거를 붙인다
- 디버그 실행의 환경 변수는 `.vscode/settings.json`의 `cmake.debugConfig.environment`에 넣는다
  (`.vscode/`는 git에서 제외한다)
- VS Code가 `QT_ROOT_DIR`을 모르면 터미널에서 `code .`로 열거나 `CMakeUserPresets.json`을 쓴다
- 선택: The Qt Company의 VS Code 확장(Qt Extension Pack) — 디버거의 Qt 타입 표시, `.ui`/`.qrc` 지원

### CLion (선택)

- `CMakePresets.json`을 자동으로 읽는다. 프리셋 프로필을 켜고 기본 `Debug` 프로필(`cmake-build-debug/`)은 끈다
- GUI로 실행하면 `QT_ROOT_DIR`을 모르므로 `CMakeUserPresets.json`이 필요하다
- `.idea/`, `cmake-build-*/`는 git에서 제외되어 있다

### Qt Creator (선택)

- `CMakeLists.txt`를 열면 프리셋을 인식한다
- `.ui` 파일 편집기(Designer), Qt 문서 통합(F1)이 강점이다

## 7. 옵션

| CMake 옵션 | 기본 | 의미 |
|---|---|---|
| `POKESIX_BUILD_TESTS` | top-level이면 ON | 테스트 빌드 |
| `POKESIX_ENABLE_CLANG_TIDY` | OFF | 컴파일하면서 clang-tidy 실행 |
| `POKESIX_WARNINGS_AS_ERRORS` | OFF | 경고를 에러로 처리 |

```bash
cmake --preset linux-debug -DPOKESIX_ENABLE_CLANG_TIDY=ON
```

## 8. 설치 · 패키징

지금은 최소 `install()`만 있다.
```bash
cmake --install build/linux-release --prefix ~/.local
```
Linux에서는 `bin/PokeSix`, `share/applications/*.desktop`, `share/icons/hicolor/`가 설치된다.
Qt 런타임은 함께 설치하지 않는다. 배포 패키지(dmg / zip / AppImage)가 필요해지면
`qt_generate_deploy_app_script()`를 붙인다.
