# A1 — MainWindow와 디버깅

> 학습 루프 ① 가이드. 코드는 직접 작성하고, 끝나면 "A1 진단해줘"라고 요청한다.

## 목표

직접 만든 `MainWindow` 클래스로 앱을 띄운다. **이 창이 Phase A 내내 홈 화면을 올릴 틀**이 된다(A2에서 그 안에 홈 페이지를 배치한다).
그 과정에서 다음을 손에 익힌다.

- VS Code(CMake Tools)가 프리셋으로 빌드·실행·디버깅하도록 연결하기
- `Q_OBJECT`와 moc가 실제로 무엇을 만드는지
- Qt의 객체 소유권(object tree)
- `QLoggingCategory`로 로그를 찍고 켜고 끄기
- 디버거로 생성자에서 멈추고 호출 스택 읽기

**눈에 보이는 결과**: "PokeSix 0.0.1" 제목의 1440×900 창(버전은 `CMakeLists.txt` 값). 960×640보다 작게 줄어들지 않는다.
터미널에 `pokesix.ui: MainWindow created` 로그가 찍힌다.

---

## 0. 준비

1. ~~`sudo apt install libxcb-cursor0`~~ ✅ 설치 완료 — 이제 xcb platform plugin이 로드된다
2. ✅ Git 준비 완료(Claude): `main`에 Phase 0 커밋 + `v0.0.1` 태그, 작업 브랜치 `feat/a1-mainwindow`로 전환됨.
   코드만 쓰면 된다. 커밋·merge는 진단이 끝나면 Claude가 한다
3. VS Code 연결 (CMake Tools + C/C++ 확장, gdb — 이미 설치되어 있다)
   - 명령 팔레트 → `CMake: Select Configure Preset` → `linux-debug`,
     `CMake: Select Build Preset` → `linux-debug`
   - `CMake: Build` (F7), `CMake: Run Without Debugging` (Shift+F5), `CMake: Debug` (Ctrl+F5).
     `launch.json` 없이 CMake Tools가 gdb로 실행한다
   - configure가 `Qt6_DIR-NOTFOUND`로 실패하면 VS Code가 `QT_ROOT_DIR`을 모르는 것이다.
     터미널에서 `code .`로 다시 열거나 `CMakeUserPresets.json`을 쓴다([build.md §1](../build.md#1-핵심-개념-qt_root_dir))
   - 지금의 빈 창이 VS Code에서 뜨는지 먼저 확인한다

## 1. 알아야 할 개념

### 1-1. `Q_OBJECT`와 moc

`Q_OBJECT`는 매크로다. 펼치면 클래스 안에 **선언만** 몇 개 들어간다.
```cpp
static const QMetaObject staticMetaObject;
const QMetaObject *metaObject() const override;
int qt_metacall(QMetaObject::Call, int, void **) override;
// ...
```
이 함수들의 **정의**는 moc가 헤더를 읽고 `moc_mainwindow.cpp`에 생성한다. 그래서 두 가지가 따라온다.
- 헤더가 AUTOMOC 스캔 대상에 없으면 선언만 있고 정의가 없다
  → 링크 에러 `undefined reference to 'vtable for MainWindow'`
- `QMetaObject`에는 클래스 이름, 시그널·슬롯 목록, 프로퍼티가 들어 있다.
  A6에서 배울 시그널/슬롯이 모두 이 위에서 동작한다

ROS 2에 빗대면 `.msg` 파일에서 `rosidl`이 타입 지원 코드를 생성하는 것과 같다.
차이는 moc의 입력이 **C++ 헤더 자체**라는 점이다.

### 1-2. `QMainWindow`

메뉴 바, 툴바, 도킹 영역, 상태 바, 그리고 가운데 **central widget** 자리를 제공하는 틀이다.
PokeSix는 메뉴 바나 툴바 대신 커스텀 `AppBar`를 쓸 것이므로 결국 central widget만 쓴다(A2에서 채움).
A1에서는 창 자체의 속성(제목, 크기, 최소 크기)만 다룬다.

### 1-3. Object tree — 누가 delete하는가

```cpp
auto *label = new QLabel(this);   // 부모 = this. delete 하지 않는다
```
`QObject`는 부모가 소멸할 때 자식을 전부 delete한다. 그래서 Qt 코드에는 `new`가 많지만
누수가 아니다. 부모가 없는 **최상위 객체**(지금은 `MainWindow`)만 수명을 직접 관리한다.
`main()`의 스택에 두면 `app.exec()`가 반환된 뒤 소멸한다.

생성자 시그니처 관례는 `explicit MainWindow(QWidget *parent = nullptr);`다. 왜 `explicit`이고
왜 `parent`를 받는지 생각해 보자(힌트: 나중에 이 창을 다른 위젯의 자식으로 만들 일은 없어도, 관례를 지키면
모든 위젯이 같은 모양이 된다).

### 1-4. `QLoggingCategory`

[conventions.md §6](../conventions.md#6-로깅)의 패턴을 쓴다. 정할 것:
- 카테고리 선언(`Q_DECLARE_LOGGING_CATEGORY`)을 **어느 헤더**에 둘지. ui 레이어 전체가 공유할
  작은 헤더 하나를 추천한다
- `qCInfo`와 `qCDebug`의 차이: 카테고리를 `Q_LOGGING_CATEGORY(이름, "문자열", QtInfoMsg)`처럼 **세 번째 인자와 함께** 정의하면
  debug는 기본으로 꺼지고 `QT_LOGGING_RULES`로 켤 때만 나온다. 세 번째 인자를 빼면 debug도 기본으로 켜진다

### 1-5. compile definition의 범위

지금 `main.cpp`는 `POKESIX_VERSION` 매크로를 쓴다. 이 정의는 `PokeSix` **실행 파일 타깃에만**
걸려 있다(`target_compile_definitions(PokeSix PRIVATE …)`). 새로 만들 `pokesix_ui` 타깃에서는
보이지 않는다. MainWindow가 버전을 알아야 할 때 어떻게 할지 두 가지 방법을 비교해 보자.
- 매크로를 ui 타깃에도 건다
- 런타임 값 `QApplication::applicationVersion()`을 읽는다

## 2. 작업 순서

1. `src/ui/shell/`에 `mainwindow.h` / `mainwindow.cpp`를 만든다
   - `QMainWindow`를 상속하고 `Q_OBJECT`를 붙인다
   - 생성자에서 창 제목, 기본 크기(1440×900), 최소 크기(960×640)를 설정한다
     (최소 크기는 설계서 §7 `setMinimumSize(960, 640)`)
   - 생성 완료 로그를 `qCInfo`로 찍는다
2. 로깅 카테고리 헤더/소스를 만든다(`pokesix.ui`)
3. `src/ui/CMakeLists.txt`의 템플릿 주석을 풀어 `pokesix_ui` 타깃을 만든다
   - 아직 `pokesix_data`가 없으니 그 링크 줄은 빼 둔다
4. `src/CMakeLists.txt`: `PokeSix`가 `Qt6::Widgets` 대신 `pokesix_ui`를 링크하게 바꾼다
   - `Qt6::Widgets`는 `pokesix_ui`의 PUBLIC 의존성이라 자동으로 따라온다. 왜 그런지 설명할 수 있으면 된다
5. `main.cpp`에서 `QMainWindow` 대신 `MainWindow`를 쓴다
6. 빌드 경고 0, 포맷 위반 0을 확인한다

## 3. 실험 (진단 때 결과를 알려 줄 것)

1. **디버거**: `MainWindow` 생성자 첫 줄에 breakpoint를 걸고 Debug로 실행한다
   - 호출 스택에서 `main` → `MainWindow::MainWindow`를 확인한다
   - `this`를 펼쳐 `QMainWindow` 부모 부분이 보이는지 확인한다
   - 참고: 기본 gdb 표시로는 `QString` 내용이 바로 보이지 않는다. 필요하면 The Qt Company의
     VS Code 확장(Qt Extension Pack)이 Qt 타입 표시를 지원한다. 선택 사항이다
2. **moc 결과물 읽기**: 빌드 폴더에서 `moc_mainwindow.cpp`를 찾는다
   (`build/linux-debug/src/ui/pokesix_ui_autogen/` 아래). 그 안에서 클래스 이름 문자열과
   `staticMetaObject` 정의를 찾아본다
3. **`Q_OBJECT` 빼 보기**: 생성자에서 `metaObject()->className()`을 로그로 찍는다.
   `Q_OBJECT`를 지우고 다시 빌드·실행한 뒤 출력이 어떻게 바뀌는지 보고, 원인을 한 줄로 설명한다.
   확인이 끝나면 되돌린다
4. **로그 켜고 끄기**: `qCDebug` 줄을 하나 추가한다. 그냥 실행했을 때와
   `QT_LOGGING_RULES="pokesix.*.debug=true"`로 실행했을 때를 비교한다.
   터미널에서 앞에 붙여 실행하거나, VS Code에서는 `.vscode/settings.json`의 `cmake.debugConfig.environment`에 넣는다(`.vscode/`는 git에서 제외됨)

## 4. 완료 조건

- [ ] VS Code에서 `linux-debug` 프리셋으로 빌드·실행·디버깅(Ctrl+F5)된다
- [ ] 제목 "PokeSix 0.0.1", 1440×900 창이 뜨고 960×640보다 작아지지 않는다
- [ ] `pokesix.ui` 카테고리 info 로그가 찍히고, debug 로그는 규칙으로만 켜진다
- [ ] 빌드 경고 0, `clang-format --dry-run --Werror` 통과, `ctest` 통과
- [ ] 실험 1~4 결과를 설명할 수 있다

## 5. 진단 요청할 때

```
A1 진단해줘
```
진단은 `git diff main...feat/a1-mainwindow`(커밋 전 변경 포함), 빌드 출력, 실행 로그를 보고 한다.
막혔으면 에러 메시지 전체를 붙여 준다(요약하지 말 것). 진단을 통과하면 Claude가 커밋하고 `main`에 merge한다.

## 6. 막히면 먼저 볼 것

| 증상 | 의심할 곳 |
|---|---|
| `undefined reference to 'vtable for MainWindow'` | 헤더가 타깃 소스 목록에 있는가(AUTOMOC 대상인가) |
| `'POKESIX_VERSION' was not declared` | §1-5 |
| `Qt6_DIR-NOTFOUND` (VS Code) | VS Code가 `QT_ROOT_DIR`을 모름 → 터미널에서 `code .` 또는 `CMakeUserPresets.json` |
| 창이 안 뜨고 `xcb` 에러 | [build.md §5-2](../build.md#platform-plugin) |
| `fatal error: QMainWindow: No such file` (ui 타깃) | ui 타깃이 `Qt6::Widgets`를 링크하는가 |

## 7. 읽을 문서

- [QMainWindow](https://doc.qt.io/qt-6/qmainwindow.html) — "Qt Main Window Framework" 그림
- [The Meta-Object System](https://doc.qt.io/qt-6/metaobjects.html), [Using the Meta-Object Compiler (moc)](https://doc.qt.io/qt-6/moc.html)
- [Object Trees & Ownership](https://doc.qt.io/qt-6/objecttrees.html)
- [QLoggingCategory](https://doc.qt.io/qt-6/qloggingcategory.html) — "Configuring Categories"
- [VS Code CMake Tools — Debug](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/debug-launch.md)
- [CMake AUTOMOC](https://cmake.org/cmake/help/latest/prop_tgt/AUTOMOC.html) — 헤더 스캔 규칙
