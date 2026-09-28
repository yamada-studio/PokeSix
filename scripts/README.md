# scripts

OS마다 폴더가 따로 있고, 각 폴더는 자기 OS만 다룬다(다른 OS에서 실행하면 바로 거부한다).

```
scripts/
├── QT_VERSION        Qt 버전 (세 OS 스크립트가 모두 이 파일을 읽는다)
├── linux/            bash · Ubuntu 24.04 (apt)
│   ├── env.sh        공통 설정 (source 전용)
│   ├── setup.sh      의존성 설치: apt 패키지(빠진 것만 sudo) + Qt(aqtinstall)
│   ├── build.sh      configure → build → test (→ install)
│   └── run.sh        실행 (없으면 빌드부터)
├── macos/            bash 3.2 호환 · Xcode CLT + Homebrew
│   └── env.sh  setup.sh  build.sh  run.sh
└── windows/          cmd (.bat) · MSVC 2022 · winget
    └── env.bat setup.bat build.bat run.bat
```

## 옵션 요약

| | Linux / macOS | Windows |
|---|---|---|
| `setup` | `--no-system` · `--no-qt` | `--no-system` · `--no-qt` · `--yes` (VS 설치 확인 생략) |
| `build` | `debug`/`release` · `--clean` · `--no-test` · `--format` · `--tidy` · `--install <prefix>` | `debug`/`release` · `--clean` · `--no-test` · `--install <prefix>` |
| `run` | `debug`/`release` · `--rebuild` · `--log` · `--offscreen` · `--gdb`(Linux) / `--lldb` · `--open`(macOS) · `-- <앱 인자>` | `debug`/`release` · `--rebuild` · `--log` · `--offscreen` · `--vs` · `-- <앱 인자>` |

## OS별 참고

- **Linux**: `setup.sh`는 빠진 apt 패키지가 있을 때만 sudo를 요청한다
- **macOS**: 기본 bash 3.2에서 동작하도록 작성했다(`mapfile` 금지, 빈 배열은 `${a[@]+"${a[@]}"}`로 전개)
- **Windows**
  - Developer Command Prompt가 필요 없다. CMake가 PATH에 없으면 VS 2022에 번들된 CMake를 `vswhere`로 찾는다
  - PokeSix는 GUI(WIN32) 앱이라 콘솔 출력이 없다. `run.bat`은 로그를 `build\windows-msvc\PokeSix-<config>.log`에 받아 앱이 끝난 뒤 보여 준다. 실행 중에 보려면 `--vs`를 쓴다
  - 한국어 Windows 콘솔(CP949)에서는 UTF-8 한글이 깨진다. 스크립트를 영어로 쓰는 이유 중 하나다

## 검증 상태

| | 실행 검증 |
|---|---|
| linux | ✅ setup · build(debug/release, `--format`, `--install`) · run(`--offscreen`) · shellcheck |
| macos | ⚠ shellcheck(bash)만 통과. macOS 실기 미검증 |
| windows | ⚠ Wine의 cmd로 인자 파싱·에러 경로·run 흐름(인자 전달, 로그, exit code)만 확인. 실제 VS/winget/Qt 설치 미검증 |

## 스크립트를 고칠 때

- **스크립트(`.sh`, `.bat`)는 주석·사용법·출력 메시지 모두 영어로 쓴다**
- Qt 버전 변경은 `QT_VERSION` 한 줄만 고친다
- `.sh`: `shellcheck -x -P SCRIPTDIR scripts/*/*.sh`(`uvx --from shellcheck-py shellcheck …`)
- `.bat`: `( )` 블록 안에서 경로 변수를 펼치지 않는다(`Program Files (x86)`의 `)`). 분기는 `goto`로 처리한다
