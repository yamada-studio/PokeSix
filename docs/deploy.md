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
PokeSix --screenshot <screen> <WxH> <out.png> [--screenshot-delay <ms>]
```

| 인자 | 뜻 |
|---|---|
| `--language <code>` | 화면 문구와 게임 데이터 이름의 언어. 없으면 `ko`. 고른 값은 저장되어 다음 실행에도 남는다([ADR 0013](decisions/0013-localization.md)) |
| `--screenshot <screen> <WxH> <out.png>` | 창을 그 크기로 띄워 그 화면을 연 뒤 PNG로 찍고 종료한다(0 성공 · 1 저장 실패 · 2 인자 오류). `screen` = `intro` · `dex` · `items` · `map` · `squad` · `settings`. 디자인 기준 이미지(`design/handoff-v2/images/screens/`)와 나란히 비교하는 진단 도구(A3 · A10). 디스플레이가 없으면 `--offscreen`(run 스크립트) 또는 `QT_QPA_PLATFORM=offscreen` |
| `--screenshot-delay <ms>` | 찍기 전 대기(기본 1200). 인트로의 카드 부채꼴 등장 애니메이션(760ms)이 끝난 뒤 찍기 위한 값 |
| `--help` / `--version` | Qt의 `QCommandLineParser`가 처리한다. **Windows GUI 빌드에서는 콘솔이 없어 메시지 상자로 뜬다** |

```bat
scripts\windows\run.bat release --offscreen -- --screenshot intro 1440x900 %TEMP%\intro.png
```
```bash
scripts/linux/run.sh release --offscreen -- --screenshot squad 1440x900 /tmp/squad.png
```
캡처에는 스프라이트가 찍히므로 **리포에 커밋하지 않는다**(CLAUDE.md §7). 첫 실행(DB 없음) 상태면 인트로에 FirstRunPanel이 보이고 다른 화면은 비어 있다.

예정된 인자(로드맵): `--gallery`(Phase B), `--overlay [sav]`([overlay-design.md](overlay-design.md)).
인자를 더할 때는 `src/app/application.cpp`의 `parseArguments()`에 `QCommandLineOption`을 추가하고 이 표를 갱신한다.

로그 한 줄의 형식(`Application::setupLogging`): `hh:mm:ss.zzz L category: message` — `L`은 수준 한 글자(D · I · W · E · F). 예:
```
18:46:51.839 I pokesix.ui: MainWindow initialized
```

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

### 2-3. 방안 B — CMake `install`에 넣기 (**적용됨**, 2026-10-07)

`src/CMakeLists.txt`의 `install(TARGETS PokeSix …)` 다음에 Qt의 배포 스크립트가 붙어 있다:

```cmake
qt_generate_deploy_app_script(
    TARGET PokeSix
    OUTPUT_SCRIPT pokesix_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR   # Linux는 deploy를 지원하지 않는다 → 거기서는 조용히 건너뛴다
    NO_TRANSLATIONS
    ${POKESIX_DEPLOY_OPTIONS}       # Windows에서만: --compiler-runtime · 플러그인 제외(§2-2의 표)
)
install(SCRIPT ${pokesix_deploy_script})
```

그러면 기존 스크립트 옵션이 그대로 배포 폴더를 만든다:

```bat
scripts\windows\build.bat release --install C:\dist\PokeSix
```

내부적으로 `cmake --install build\windows-msvc --config Release --prefix C:\dist\PokeSix`가 돌고, Windows에서는 `bin\PokeSix.exe` 옆에 windeployqt 결과가 들어간다(macOS는 `macdeployqt`로 `.app` 안에).
플러그인 제외 · `--compiler-runtime` 같은 세부 옵션은 `DEPLOY_TOOL_OPTIONS`로 넘긴다. `cmake --install … --prefix` 폴더 전체가 곧 배포본이다.

**clone부터 실행까지 한 번에**: `scripts\windows\quickstart.bat` = setup → package.bat → 실행
(Linux는 `scripts/linux/quickstart.sh` → AppImage 실행). 패키징까지만 하려면 `--no-run`.

**ZIP 한 번에**: `scripts\windows\package.bat` — release 빌드(테스트 포함) → install + windeployqt →
LICENSE · THIRD_PARTY_NOTICES.md 동봉 → `build\windows-msvc\package\PokeSix-<버전>-win64.zip` + SHA-256.
기존 빌드를 재사용하려면 `--no-build`.

**Windows 실기 검증(2026-10-07)**: `quickstart.bat --no-run`이 끝까지 통과했다 — ctest 96개, `PokeSix-0.0.1-win64.zip` **17.6MB**.
패키지 구조는 `bin\`(exe · Qt DLL 6 · `qt.conf`의 `Prefix = ..`) + `plugins\`(platforms · sqldrivers · tls · imageformats …) + 라이선스 2개이고,
`bin\PokeSix.exe`는 PATH에서 Qt를 뺀 상태로도 떴다. 서명은 없으므로 §2-4의 SmartScreen 안내가 그대로 적용된다.

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

### 2-7. 방안 D — CI에서 자동으로 (**적용됨**, 2026-10-07)

[.github/workflows/release.yml](../.github/workflows/release.yml): 태그(`v*`)를 푸시하면 3 OS에서
`scripts/<os>/package.*`를 돌려 AppImage · ZIP · dmg를 만들고 GitHub Release에 첨부한다
(`workflow_dispatch`로 태그 없이 아티팩트만 확인할 수도 있다). Qt는 `scripts/QT_VERSION`을 읽어
setup과 같은 최소 아카이브만 받고, Linux는 ubuntu-22.04에서 빌드해 AppImage의 glibc 호환을 넓힌다.

**첫 실행 검증(2026-10-07, `workflow_dispatch`)**: 3 OS 모두 통과 — AppImage 38.9MB · dmg 31.4MB · ZIP 17.5MB, 소요 Linux 3.5분 · macOS 2.5분 · Windows 7.5분(Qt 캐시 후). 첫 시도에서 고친 것 둘:
macOS 러너(AppleClang 15)는 람다의 구조적 바인딩 캡처를 거부하고, `windows-latest`는 이제 VS 2026 이미지라 **`windows-2022`로 고정**했다(프리셋이 "Visual Studio 17 2022").

**태그 경로(`v0.1.0`, 2026-10-07)**: 패키지는 다 만들었지만 Release 첨부가 403으로 실패했다 — 조직이 `GITHUB_TOKEN` 기본 권한을 read로 고정해서다.
워크플로에 `permissions: contents: write`를 선언했다(다음 태그부터 적용). v0.1.0 Release는 그 실행의 아티팩트를 받아 손으로 올렸다. 그래도 403이면 조직 설정 "Actions → Workflow permissions"를 read/write로 바꾸거나, PAT를 secret으로 넣어 `token:`에 넘긴다.
**사용자는 Releases에서 파일 하나만 받으면 된다 — Qt · 컴파일러 · setup 불필요.**
아직 실제 러너에서 돌려 보지 않았다: 첫 태그 때 로그를 보고 다듬는다.

### 2-8. 다른 OS 한 줄씩

- **macOS** (**적용됨**, 2026-10-07): `scripts/macos/package.sh` — release 빌드(테스트 포함) →
  `cmake --install`(macdeployqt가 `.app`에 프레임워크를 넣는다) → LICENSE · 고지문 · /Applications
  링크를 담아 `hdiutil`로 `build/package/PokeSix-<버전>-macos.dmg` + SHA-256.
  `scripts/macos/quickstart.sh` = setup → package → 실행. 서명 · 공증(Apple Developer 연 99달러)이
  없어 Gatekeeper가 경고한다 — 처음 한 번 우클릭 → 열기
- **Linux** (**적용됨**, 2026-10-07): `scripts/linux/package.sh` — release 빌드(테스트 포함) →
  `build/package/AppDir`에 install → `linuxdeploy` + `linuxdeploy-plugin-qt`(처음 한 번
  `build/package/tools`에 받아 둔다)로 `build/package/PokeSix-<버전>-x86_64.AppImage` + SHA-256.
  TLS 백엔드는 자동 감지에 안 걸려서 `EXTRA_PLUGINS="tls;networkinformation"`으로 넣는다(첫 실행
  HTTPS 데이터 받기). 기존 빌드 재사용은 `--no-build`, `--install`을 주면 앱 메뉴에도 등록한다
  (`~/.local/bin/PokeSix.AppImage` + `.desktop`의 Exec 절대 경로 + hicolor 아이콘 — 옛
  `--install ~/.local` 설치가 남긴 메뉴 항목을 덮어쓴다). AppImage는 `chmod +x` 뒤 더블클릭(또는
  `./PokeSix-….AppImage`)으로 실행되고, 빌드한 배포판보다 오래된 glibc에서는 안 돈다.
  Flatpak은 KDE 런타임에 Qt 6.8이 있어 대안이 된다
