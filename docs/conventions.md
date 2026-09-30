# 코드 · 프로젝트 컨벤션

## 1. 포맷

- 기준은 [`.clang-format`](../.clang-format) (Qt 스타일: 4칸, 100열, `Type *ptr`, 클래스·함수 중괄호만 새 줄)
- 손으로 맞추지 않는다. 저장 시 IDE 포맷 또는 `clang-format -i <files>`
- 정적 분석: [`.clang-tidy`](../.clang-tidy). 빌드에 붙이려면 `-DPOKESIX_ENABLE_CLANG_TIDY=ON`

## 2. 네이밍 (Qt 관례)

| 대상 | 규칙 | 예 |
|---|---|---|
| namespace | `com::yamada::studio` 하나 | `namespace com::yamada::studio {` |
| 클래스 · 구조체 · enum | `PascalCase` | `TypeChart`, `SquadSlotCard` |
| enum 값 | `PascalCase` (`enum class`만 사용) | `Type::Fire`, `WidthClass::Narrow` |
| 함수 · 메서드 | `camelCase` | `multiplier()`, `setGeneration()` |
| 지역 변수 · 매개변수 | `camelCase` | `attacker`, `slotIndex` |
| private / protected 멤버 | `m_` + `camelCase` | `m_generation` |
| public 멤버 (struct) | `camelCase` | `species.dexNo` |
| 상수 (`constexpr`) | `camelCase` | `typeCount` |
| getter / setter | Qt식: getter는 이름 그대로, setter는 `set` 접두사 | `generation()` / `setGeneration()` |
| bool getter | `is` / `has` / `can` | `isEmpty()`, `hasSelection()` |
| 시그널 | 과거분사 또는 `~Changed` | `generationChanged`, `selectionChanged`, `finished` |
| 슬롯 | 동사형 | `refresh()`, `applyTheme()` |
| 로깅 카테고리 변수 | `lc` + 레이어 | `lcUi`, `lcDataApi` |

- 네이밍은 clang-tidy `readability-identifier-naming`이 검사한다.
- 디자인 핸드오프의 `Tokens.h`는 `kRed`식 이름을 쓴다. 옮길 때 이 규칙(`camelCase`)으로 바꿀지는
  A4 단계에서 정한다.

## 3. 파일

- 파일명은 **소문자**, 클래스명과 같게: `MainWindow` → `mainwindow.h` / `mainwindow.cpp`
- 한 파일에 공개 클래스 하나
- 헤더 가드는 `#pragma once`
- 테스트: `<대상>_test.cpp` (`typechart_test.cpp`)
- include 경로는 `src/` 기준: `#include "core/types/typechart.h"`
- include 순서(clang-format이 정렬): 대응 헤더 → 프로젝트 → Qt → gtest → 표준

## 4. Qt 코드 규칙

- **`connect`는 함수 포인터 문법만** 쓴다. 컴파일 타임에 검사되고 namespace 문제가 없다.
  ```cpp
  connect(state, &AppState::generationChanged, this, &DexPage::reload);
  ```
  `SIGNAL()` / `SLOT()` 문자열 매크로는 금지한다(런타임에야 실패하고, namespace가 붙은 타입에서 조용히 깨진다).
- 람다를 connect할 때는 **context 객체(3번째 인자)를 반드시 넘긴다.** context가 소멸하면 연결도 자동 해제된다.
  ```cpp
  connect(button, &QPushButton::clicked, this, [this] { ... });   // this = context
  ```
- `QObject`를 상속하고 시그널/슬롯/프로퍼티를 쓰는 클래스에는 `Q_OBJECT`를 붙인다.
  헤더는 CMake 타깃의 소스 목록에 넣는다(AUTOMOC 스캔 대상).
- 위젯 소유권은 **부모-자식(object tree)** 으로 관리한다. `new QLabel(this)`처럼 부모를 넘기면
  delete하지 않는다. 부모가 없는 최상위 객체만 스택이나 `std::unique_ptr`로 관리한다.
- 생성자는 `explicit`이고 마지막 인자로 `QWidget *parent = nullptr`(또는 `QObject *parent`)를 받는다.
- **상속받은 멤버 함수를 호출할 때 기반 클래스 이름을 붙인다**: `QMainWindow::setWindowTitle(...)`.
  자기 클래스의 멤버와 물려받은 멤버를 코드에서 구분하려는 목적이다(프로젝트 결정).
  - **단, 가상 함수에는 붙이지 않는다.** `Base::f()`는 가상 디스패치를 끄므로 오버라이드가 무시된다
  - 예외: 오버라이드 안에서 기반 구현을 **일부러** 부를 때(`QMainWindow::resizeEvent(event);`)
  - clang-tidy `bugprone-parent-virtual-call`이 일부 실수를 잡는다
- 문자열 리터럴: 번역 대상이면 `tr("...")`, 아니면 `QStringLiteral("...")` / `u"..."_s`.
- 색 · 크기 값을 코드에 하드코딩하지 않는다. `ui/theme`의 토큰에서만 가져온다(다크 테마 전환 대비).

## 5. 국제화 (i18n)

- 사용자에게 보이는 문구는 전부 `tr()`로 감싼다.
- **소스 언어는 한국어**다. 디자인의 문구(해요체)를 그대로 쓴다. 영어·일본어는 `translations/pokesix_en.ts`
  · `pokesix_ja.ts`(Qt Linguist, `qt_add_translations`)에 있다.
- **`tr()` 문구를 더하거나 바꾸면** `cmake --build --preset linux-debug --target update_translations`로 `.ts`를
  갱신하고, 새 줄(`type="unfinished"`)에 영어 · 일본어를 채운다. 비어 있으면 그 언어에서 한국어가 보인다.
- 포켓몬·기술·아이템 **이름**은 번역이 아니라 데이터다. PokéAPI의 다국어 이름을 DB에 저장하고
  `AppState::language`로 고른다(`LocalizedText::text()` — 빈 언어는 정해진 순서로 대신한다).
- 설정 파일(`resources/theme/*.json`)의 화면 글자(도감 이름 바꾸기 · 아이템 묶음 이름)는 `{ko, en, ja}`로 쓴다.

## 6. 로깅

- `qDebug()`를 직접 쓰지 않고 `QLoggingCategory`를 쓴다.
  ```cpp
  // 헤더:  Q_DECLARE_LOGGING_CATEGORY(lcUi)
  // .cpp:  Q_LOGGING_CATEGORY(lcUi, "pokesix.ui", QtInfoMsg)   // 3번째 인자: 기본 출력 수준
  qCInfo(lcUi) << "MainWindow created";
  ```
- 세 번째 인자 `QtInfoMsg`를 꼭 준다. 생략하면 **debug까지 기본으로 켜진다.** 이 인자를 주면 기본은 info 이상만 출력하고,
  debug는 `QT_LOGGING_RULES`로 켤 때만 나온다
- 카테고리 이름: `pokesix.app`, `pokesix.ui`, `pokesix.data.api`, `pokesix.data.db`, `pokesix.data.state` …
- 켜고 끄기: `QT_LOGGING_RULES="pokesix.*.debug=true" ./PokeSix`
- core는 Qt가 없으므로 로그를 찍지 않는다. 결과를 반환하고, 호출한 쪽(data)이 기록한다.

## 7. 에러 처리

[architecture.md §7](architecture.md#7-에러-처리) 참고. 요약하면 다음과 같다.
- 슬롯과 이벤트 핸들러 밖으로 예외를 던지지 않는다.
- core는 반환값으로, 비동기 data는 시그널로 실패를 알린다.

## 8. 설정 (QSettings)

- 형식은 모든 OS에서 `QSettings::IniFormat`(Windows 레지스트리 대신 파일. 열어 보고 지우기 쉽다).
- 키는 `group/key`, `camelCase`. 화면 정의서 SCR-05의 저장 키를 따른다.

| 키 | 타입 | 기본값 | 출처 |
|---|---|---|---|
| `meta/schemaVersion` | int | 1 | 설정 형식 변경 시 마이그레이션용 |
| `rules/gen` | int | 4 | SCR-05 기준 세대 |
| `rules/splitRule` | string | `auto` | `auto` / `type` / `move` |
| `rules/hideFairyBeforeGen6` | bool | true | |
| `rules/steelLegacyResist` | bool | true | |
| `rules/useHistoricTypes` | bool | true | |
| `rules/useHistoricStats` | bool | true | |
| `data/checkOnStart` | bool | true | |
| `lang/nameLang` | string | `ko` | `ko` / `en` / `ja` |
| `lang/showOriginalName` | bool | false | |
| `theme/mode` | string | `light` | `light` / `dark` |
| `theme/density` | string | `standard` | `dense`(30) / `standard`(34) |
| `window/geometry` | bytes | — | `saveGeometry()` |

기본값은 디자인 목업 기준 추정값이다. 설정 화면을 구현할 때 확정한다.

## 9. 리소스 (qrc)

- `resources/resources.qrc` 하나를 실행 파일에 붙인다. 정적 라이브러리에 qrc를 넣으면
  `Q_INIT_RESOURCE()`가 필요하므로 그렇게 하지 않는다.
- qrc는 `qt_add_big_resources`로 컴파일한다(글꼴 12 MB를 거대한 생성 `.cpp` 없이 오브젝트로 넣는다).
- 가상 경로: `:/styles/app.qss`, `:/fonts/*.ttf`, `:/icons/mark/…`(마크 SVG), `:/icons/window/pokesix-<크기>.png`(창 아이콘).
  원본 파일 위치와 다른 짧은 경로는 qrc의 `alias`로 붙인다.
- **자체 제작이거나 라이선스가 확인된 파일만** 넣는다(글꼴은 OFL, 라이선스 파일 동봉).
- 포켓몬 스프라이트, 공식 아트, 공식 로고, 포켓볼 도안은 넣지 않는다. `SpritePlaceholder` 위젯을 쓴다.

## 10. 접근성 · 디자인 규칙 (디자인 핸드오프 PROMPT §5)

- 모든 인터랙티브 요소는 키보드로 도달할 수 있어야 한다. Tab 순서는 화면을 읽는 순서와 같게 한다.
- 포커스 링은 파랑 3px, 간격 2px.
- 아이콘만 있는 버튼에는 `setAccessibleName()`.
- 텍스트 대비는 4.5:1 이상. 디자인 시트의 글자색 조합을 바꾸지 않는다.
- 그림자는 전부 블러 없는 "오프셋 사각형". `QGraphicsDropShadowEffect`는 쓰지 않는다.
- QSS로 표현할 수 없는 것(창 그림자, 타입 칩, 히트맵, 종족값 바, 앱 막대 탭)은 `paintEvent`에서 `QPainter`로 그린다.

## 11. Git · 버전

전체 규칙은 [versioning-and-git.md](versioning-and-git.md)에 있다. 요약:

- 버전: SemVer. **Phase 완료 = MINOR**, 릴리스 후 버그 수정 = PATCH. 기준 값은 `CMakeLists.txt` 한 곳, 태그는 `vX.Y.Z`
- 브랜치: `main` 하나 + 짧은 작업 브랜치 `<type>/<step>-<slug>` (예: `feat/a1-mainwindow`). 실험은 `exp/`(merge 안 함)
- merge: `git merge --no-ff`, 조건은 경고 0 · ctest · clang-format · 진단 완료
- 커밋: Conventional Commits, 영어, `Refs: A1` footer
- Git LFS는 쓰지 않는다. 큰 바이너리가 드물고(글꼴 12MB는 한 번 넣고 거의 바꾸지 않음),
  LFS를 쓰면 clone하는 사람도 LFS를 설치해야 해서 "clone → build → run" 재현성이 떨어진다.
  자주 바뀌는 수 MB급 바이너리가 생기면 다시 검토한다
