# 실행 CLI와 배포 (.exe)

빌드 방법은 [build.md](build.md), 릴리스 절차(버전 · 태그 · CHANGELOG)는 [versioning-and-git.md §4](versioning-and-git.md#4-릴리스-절차).
이 문서는 **만들어진 실행 파일을 어떻게 실행하고, 어떻게 남에게 전달하는가**를 다룬다.
배포 자동화(Phase G)는 아직 없다. §2는 지금 바로 할 수 있는 방법(A)부터 CI까지(D) 순서다.

---

## 1. 실행 CLI

### 1-1. 스크립트로 실행 — `scripts/<os>/run.*`

실행 파일이 없으면 먼저 빌드한다(테스트는 건너뛴다). 옵션은 세 OS가 거의 같다.

| 옵션 | 뜻 | 비고 |
|---|---|---|
| `debug` / `release` | 어느 빌드를 실행할지 | 기본 `debug` |
| `--rebuild` | 항상 빌드하고 실행 | |
| `--log` | `pokesix.*` 디버그 로그 켜기 | `QT_LOGGING_RULES=pokesix.*.debug=true` |
| `--offscreen` | 창 없이 실행 | `QT_QPA_PLATFORM=offscreen` — CI · 캡처 |
| `--gdb` / `--lldb` / `--vs` | 디버거 아래에서 실행 | Linux / macOS / Windows(`devenv /debugexe`) |
| `--open` | (macOS) `open`으로 .app 실행 | |
| `-- <앱 인자>` | `--` 뒤는 그대로 앱에 넘긴다 | 아래 1-2 |

Windows만 다른 점: PokeSix는 GUI(WIN32) 서브시스템이라 **콘솔이 없다.** `run.bat`은 `QT_FORCE_STDERR_LOGGING=1`로 로그를 stderr에 내보내게 한 뒤
`build\windows-msvc\PokeSix-<config>.log`에 받아 두고, 앱이 끝나면 출력한다. 실행 중에 보려면 `--vs`(Visual Studio 출력 창)를 쓴다.

### 1-2. 앱 인자

```
PokeSix [--language ko|en|ja] [-h|--help] [-v|--version]
```

| 인자 | 뜻 |
|---|---|
| `--language <code>` | 화면 문구와 게임 데이터 이름의 언어. 없으면 `ko`. 고른 값은 저장되어 다음 실행에도 남는다([ADR 0013](decisions/0013-localization.md)) |
| `--help` / `--version` | Qt의 `QCommandLineParser`가 처리한다. **Windows GUI 빌드에서는 콘솔이 없어 메시지 상자로 뜬다** |

예정된 인자(로드맵): `--screenshot <화면> <WxH> <out.png>`(A3), `--gallery`(Phase B), `--overlay [sav]`([overlay-design.md](overlay-design.md)).
인자를 더할 때는 `src/main.cpp`의 parser에 `QCommandLineOption`을 추가하고 이 표를 갱신한다.

Qt 공통 인자 · 환경 변수도 그대로 먹는다. 자주 쓰는 것:

| | 뜻 |
|---|---|
| `-platform offscreen` 또는 `QT_QPA_PLATFORM=offscreen` | 디스플레이 없이 실행 |
| `QT_LOGGING_RULES="pokesix.*.debug=true"` | 레이어별 로그(`pokesix.ui` · `pokesix.data` …) |
| `QT_LOGGING_RULES="qt.qpa.plugin=true"` 또는 `QT_DEBUG_PLUGINS=1` | 플랫폼 · SQL · TLS 플러그인 탐색 과정 |
| `QT_SCALE_FACTOR=1.5` | HiDPI 흉내 |

### 1-3. 앱이 쓰는 파일 위치

조직 이름 `YamadaStudio`, 앱 이름 `PokeSix`(`QStandardPaths` · `QSettings::IniFormat`). 전부 지우면 첫 실행 상태로 돌아간다.

| 무엇 | Windows | Linux | macOS |
|---|---|---|---|
| 게임 DB `pokesix.sqlite` · 스쿼드 `squads.json` | `%APPDATA%\YamadaStudio\PokeSix\` | `~/.local/share/YamadaStudio/PokeSix/` | `~/Library/Application Support/YamadaStudio/PokeSix/` |
| 캐시(PokéAPI CSV 원본 · 스프라이트) | `%LOCALAPPDATA%\YamadaStudio\PokeSix\cache\` | `~/.cache/YamadaStudio/PokeSix/` | `~/Library/Caches/YamadaStudio/PokeSix/` |
| 설정(`.ini`) | `%APPDATA%\YamadaStudio\PokeSix.ini` | `~/.config/YamadaStudio/PokeSix.ini` | `~/.config/YamadaStudio/PokeSix.ini` |

캐시는 지워도 다음 실행에 다시 받는다(첫 실행과 같은 다운로드). DB는 캐시에서 다시 만든다. 스쿼드와 설정만 백업 대상이다.

---

## 2. Windows `.exe` 배포 방안

### 2-1. 원칙

- **Release 빌드만** 배포한다(`build.bat release` → `build\windows-msvc\src\Release\PokeSix.exe`). Debug 빌드는 `Qt6Cored.dll` 같은 디버그 DLL과 디버그 CRT에 의존해 다른 PC에서 돌지 않는다
- Qt는 **LGPL 동적 링크**다(CLAUDE.md §2). 그래서 Qt DLL을 실행 파일 옆에 같이 넣어야 하고, 사용자가 DLL을 바꿔 끼울 수 있어야 한다(지금 구조가 그렇다). 배포본에 Qt의 LGPL 고지와 소스 입수처(qt.io)를 적는다
- **게임 에셋은 배포본에도 없다.** 데이터는 첫 실행에 PokéAPI CSV를 받아 만든다([ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md)). 따라서 첫 실행에 인터넷이 필요하다는 안내가 필요하다
- 실행 파일 하나로 합치는(static Qt) 방식은 쓰지 않는다. 상용 라이선스 또는 LGPL의 재링크 의무가 걸린다

### 2-2. 방안 A — `windeployqt`로 폴더 만들기 (지금 바로 가능)

Qt에 포함된 `windeployqt`가 실행 파일이 쓰는 Qt DLL과 플러그인을 찾아 옆에 복사한다.

```bat
scripts\windows\build.bat release
mkdir dist\PokeSix
copy build\windows-msvc\src\Release\PokeSix.exe dist\PokeSix\
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe --release --compiler-runtime ^
    --no-translations --no-system-d3d-compiler --no-opengl-sw ^
    --exclude-plugins qsqlodbc,qsqlpsql,qsqlmimer ^
    dist\PokeSix\PokeSix.exe
```

이 리포의 Debug 실행 파일로 돌려 본 결과(2026-10-04), 들어오는 것은 다음과 같다. Release는 `d` 접미사 없는 DLL이고 크기가 훨씬 작다.

```
PokeSix.exe
Qt6Core.dll  Qt6Gui.dll  Qt6Widgets.dll  Qt6Svg.dll  Qt6Sql.dll  Qt6Network.dll
platforms\qwindows.dll            ← 없으면 "platform plugin을 찾을 수 없음"(build.md §5-2)
sqldrivers\qsqlite.dll            ← 없으면 "QSQLITE driver not loaded"(build.md §5-3)
tls\qschannelbackend.dll …        ← HTTPS(첫 실행 데이터 받기). Windows 내장 schannel
imageformats\ iconengines\ styles\ generic\ networkinformation\
```

옵션의 뜻:

| 옵션 | 왜 |
|---|---|
| `--compiler-runtime` | VC++ 2022 재배포 패키지(`vc_redist.x64.exe`)를 같이 둔다. 없는 PC에서는 `VCRUNTIME140.dll` 오류가 난다 |
| `--no-translations` | Qt 자체 번역(`qtbase_ko.qm` 등)은 안 쓴다. 우리 번역은 실행 파일 안(`:/i18n`)에 있다 |
| `--no-system-d3d-compiler` · `--no-opengl-sw` | QPainter 래스터만 쓰므로 D3D 컴파일러 · 소프트웨어 OpenGL(약 20MB)이 필요 없다 |
| `--exclude-plugins qsqlodbc,qsqlpsql,qsqlmimer` | SQLite만 쓴다. 다른 SQL 드라이버는 빼도 된다 |

확인: Qt를 설치하지 않은 PC(또는 Windows Sandbox)에 `dist\PokeSix` 폴더를 복사해 실행 → 첫 실행 데이터 받기까지 되는지 본다.
플러그인을 더 빼고 싶으면 하나씩 빼고 다시 실행해 본다(`QT_DEBUG_PLUGINS=1`로 무엇을 찾는지 보인다).

### 2-3. 방안 B — CMake `install`에 넣기 (권장 · Phase G 초입)

A를 손으로 반복하지 않도록 Qt의 배포 스크립트를 `install()`에 붙인다. `src/CMakeLists.txt`의 `install(TARGETS PokeSix …)` 다음에 패턴만 적으면:

```cmake
qt_generate_deploy_app_script(
    TARGET PokeSix
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR   # Linux는 deploy를 지원하지 않는다 → 거기서는 조용히 건너뛴다
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})
```

그러면 기존 스크립트 옵션이 그대로 배포 폴더를 만든다:

```bat
scripts\windows\build.bat release --install C:\dist\PokeSix
```

내부적으로 `cmake --install build\windows-msvc --config Release --prefix C:\dist\PokeSix`가 돌고, Windows에서는 `bin\PokeSix.exe` 옆에 windeployqt 결과가 들어간다(macOS는 `macdeployqt`로 `.app` 안에).
플러그인 제외 · `--compiler-runtime` 같은 세부 옵션은 `DEPLOY_TOOL_OPTIONS`로 넘긴다. `cmake --install … --prefix` 폴더 전체가 곧 배포본이다.

여기까지 하면 **ZIP 압축 = 포터블 배포본**이다. CPack을 켜면 압축도 CMake가 한다:

```cmake
set(CPACK_GENERATOR ZIP)                       # Windows 기본. NSIS · WIX 추가 가능
set(CPACK_PACKAGE_FILE_NAME "PokeSix-${PROJECT_VERSION}-win64")
include(CPack)
# → cpack --config build\windows-msvc\CPackConfig.cmake -C Release → PokeSix-0.0.1-win64.zip
```

### 2-4. 방안 C — 설치 프로그램

| 형식 | 장점 | 단점 | 언제 |
|---|---|---|---|
| **ZIP(포터블)** | 가장 단순. 압축 풀고 실행. 설치 흔적 없음 | 시작 메뉴 · 바로 가기 없음. SmartScreen 경고(아래) | **첫 공개(v0.x)에 권장** |
| **Inno Setup** | 스크립트 한 장(`.iss`)으로 설치 · 제거 · 시작 메뉴 · 바로 가기. 무료 · 널리 쓰임 | 도구 하나 더 | v1.0 설치판 |
| NSIS (CPack 내장) | CMake만으로 생성(`CPACK_GENERATOR NSIS`) | 화면이 구식, 커스터마이즈가 번거롭다 | Inno 대신 CPack만으로 끝내고 싶을 때 |
| MSI (WiX) | 기업 배포 표준, 그룹 정책 | 가장 복잡 | 필요 없음 |
| MSIX / Microsoft Store | 자동 업데이트, 샌드박스 | **코드 서명 필수**, 패키지 ID · 스토어 계정 | 당장은 아님 |

**코드 서명.** 서명 없는 exe · 설치 파일은 처음 실행할 때 Windows SmartScreen이 "알 수 없는 게시자" 경고를 띄운다(“추가 정보 → 실행”으로 넘어갈 수 있다).
서명하려면 OV/EV 인증서(연 수십만 원) 또는 Azure Trusted Signing(월 과금)이 필요하다. 개인 팬 도구 단계에서는 서명하지 않고 README에 경고가 뜨는 이유와 해시(SHA-256)를 적는 것으로 충분하다.

### 2-5. 배포본에 같이 넣을 것

- `LICENSE`(MIT) — 앱 자체
- `THIRD-PARTY.txt` — Qt 6.8 **LGPL v3** 고지와 소스 입수처, 글꼴 라이선스(Silkscreen · 도현 · 나눔고딕: OFL), GoogleTest(BSD, 배포본에는 안 들어가지만 소스에 있다)
- `README.txt` — 첫 실행에 인터넷으로 PokéAPI 데이터를 받는다는 것, 데이터 · 설정 위치(§1-3), "비공식 팬 도구 · 데이터: PokéAPI" 표기(로드맵 열린 질문 10), SmartScreen 경고 안내
- 아이콘은 이미 실행 파일에 박혀 있다(`resources/platform/windows/pokesix.rc`). 버전 문자열(`POKESIX_VERSION`)은 `CMakeLists.txt`의 `project(VERSION)` 한 곳에서 온다

### 2-6. 배포 전 체크리스트

1. `build.bat release` → ctest 전부 통과
2. Qt가 없는 깨끗한 환경(Windows Sandbox 또는 다른 PC)에서 폴더째 실행
3. 첫 실행: 데이터 받기(HTTPS · schannel) → 변환 → 도감이 뜬다
4. 재실행: 오프라인에서도 뜬다(캐시 · DB 재사용)
5. `--language en` · `ja` 문구 확인
6. 압축 파일 이름에 버전(`PokeSix-0.0.1-win64.zip`), SHA-256을 릴리스 노트에

### 2-7. 방안 D — CI에서 자동으로 (Phase G)

GitHub Actions `windows-latest` + [`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action)(Qt 6.8.3 고정, `scripts/QT_VERSION`을 읽는다) →
`scripts\windows\build.bat release --install dist\PokeSix` → ZIP → 태그 푸시 때 GitHub Release에 첨부.
Linux(AppImage) · macOS(dmg)도 같은 워크플로의 매트릭스로. 로드맵 백로그 "GitHub Actions CI (3 OS)" 항목이 이것이다.

### 2-8. 다른 OS 한 줄씩

- **macOS**: `qt_generate_deploy_app_script`가 `macdeployqt`를 불러 `.app` 번들 안에 프레임워크를 넣는다. 배포는 `hdiutil`로 `.dmg`. Gatekeeper 때문에 공증(notarization, Apple Developer 연 99달러)이 없으면 우클릭 → 열기 안내가 필요하다
- **Linux**: Qt의 deploy 스크립트가 Linux를 지원하지 않으므로 `linuxdeploy` + `linuxdeploy-plugin-qt`로 AppImage를 만든다. 지금 `install()`은 `bin/` · `.desktop` · hicolor 아이콘을 넣고 RUNPATH에 Qt 경로를 남긴다(개발 PC용). Flatpak은 KDE 런타임에 Qt 6.8이 있어 대안이 된다
