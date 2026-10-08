# 10. 스타일이 위젯에 닿는 길 — 토큰 · QSS · 글꼴

색 · 크기를 코드에 하드코딩하지 않는다(CLAUDE.md §5). 값은 두 길로 위젯에 닿는다: **QSS**(기본 위젯) 와 **토큰 직접 사용**(손그림 위젯).

## 1. 전체 흐름

```
design/handoff-v2/…/Tokens.h  ──이식──▶  src/ui/theme/tokens.h        tok::kRed, tok::kInk …  (inline constexpr QRgb)
                                              │
              ┌───────────────────────────────┴───────────────────────────────┐
              ▼                                                               ▼
 resources/styles/app.qss  "@red" "@ink.soft" …                    손그림 위젯의 paintEvent
              │                                                    painter.setBrush(QColor(tok::kRed))
 theme::apply(app)  (Application::buildObjects의 첫 단계, 위젯이 생기기 전)
   1. loadBundledFonts()        :/fonts/*.ttf → QFontDatabase::addApplicationFont (프로세스 안에서만, 시스템 설치 아님)
   2. app.qss를 qrc에서 읽음
   3. substituteTokens()        정규식 @이름 → 표(kColors)에서 #RRGGBB로 바꿈. 모르는 이름은 경고 로그(pokesix.ui)
   4. app.setStyleSheet(…)      ★ 앱 전체에 단 한 번. 위젯별 setStyleSheet은 코드 어디에도 없다
              │
              ▼
 위젯은 objectName · 동적 속성으로 "골라진다"
   m_table->setObjectName("dexTable")        →  QTableView#dexTable { … }
   label->setProperty("state", "pending")    →  QLabel#squadSaveStatus[state="pending"] { … }
```

`QPalette` · `setPalette`는 쓰지 않는다. 다크 테마(Phase F3)는 토큰 표를 바꾸는 방식으로 계획돼 있다(`tokens.h`의 `dark` 네임스페이스, 아직 연결 안 됨).

## 2. 토큰 (tokens.h)

| 묶음 | 예 |
|---|---|
| 빨강 · 종이 · 파랑 · 초록 · 노랑 | `kRed` `kRedDeep` `kRedTint` · `kPaper` `kPaperAlt` · `kBlue` `kBlueFocusRing` · `kGreen` · `kYellow` `kYellowTint` `kYellowRowSel` |
| 먹 · 글자 · 선 | `kInk` · `kText1` `kText2` `kText3` `kTextDisabled` · `kLine` `kLineSoft` `kLineStrong` |
| 의미 색 | 능력치 `kStatLow…High` · 기술 분류 `kCatPhysical` `kCatSpecial` `kCatStatus` · 히트맵 `kHeatX4…Zero` |
| 마크 · 인트로 | `kCapsule…` · `kIntroStripe` `kIntroTitleShadow` |
| 타입 18 | `kTypes` — `TypeColor{key, ko, abbr, fill, text}` |
| 크기 | `kSizeAppBar`(60) · `kSizeTabActive`(45) · `kSizePanelHeader` · `kSizeTableRow` · `kSizeHeatCell…` · `kSizeIntro…` |
| 폭 구간 | `kBpWide` · `kBpMedium` · `kBpNarrow` · `kBpSquadSplit` … |

각 줄 끝 주석이 디자인의 tokens.json 이름 = QSS의 `@이름`이다. 글꼴은 토큰이 아니라 `theme.h`의 상수다(아래).
(위젯 안의 작은 수치 — 버튼 반경 · 안쪽 여백 — 는 아직 각 파일의 `constexpr`로 남아 있는 곳이 많다.)

## 3. QSS를 쓰는 곳과 안 쓰는 곳

| QSS로 꾸미는 것 | 직접 그리는 것(QSS 무시) |
|---|---|
| `QLabel` · `QLineEdit` · `QPushButton` · `QToolButton` · `QCheckBox` · `QScrollBar` · `QMenu` · `QTableView` · `QDialog` 바탕 | `QAbstractButton` 계열 ★ 전부, 히트맵 · 레이더 · 지도 · 슬롯 카드 · 패널 겉모양 |
| 기본 위젯에 `objectName`을 붙여 화면별로 다르게 | `tok::` 색과 `theme::font()`를 `paintEvent`에서 직접 |

직접 그리는 이유(코드 주석에서): QSS로는 **블러 없는 오프셋 그림자**를 못 그린다, 한 글자 안에서 두 색을 못 쓴다, 디자인 수치를 px 단위로 맞추기 어렵다.

**함정**: app.qss의 `QWidget { font-family: …; font-size: … }` 규칙이 `QWidget::setFont()`를 덮어쓴다. 그래서 손그림 위젯은 글꼴을 멤버(`m_font`)로 들고 `painter.setFont(m_font)`로 그린다(`ShadowButton` · `GenerationButton` · `TabButton` · `WordmarkLabel`).

## 4. app.qss의 구조 (약 200줄)

| 부분 | 선택자 |
|---|---|
| 전역 | `QMainWindow`(종이 바탕 — `QWidget`에는 일부러 안 줌) · `QWidget`(글자색 · 나눔고딕 13) · `QToolTip` · `QPushButton` · `QLineEdit` · `QCheckBox` · `QTableView` · `QHeaderView::section` · `QScrollBar` · `QMenu` · `QLabel { background: transparent }` · `QDialog` |
| 인트로 · 첫 실행 | `#introSubtitle` `#introVersion` `#introSource` · `#firstRunPages` `#firstRunTitle` `#firstRunBody` `#firstRunMeta` `#firstRunStep` `#firstRunPercent` `#firstRunNote` |
| 공용 | `#searchFieldInput`(테두리 없음 — 바깥 상자는 그림) `#searchFieldShortcut` · `#filterCaption` `#filterValue` `#itemEffect` `#previewEmpty` `#pagePlaceholder` |
| 도감 · 아이템 | `#dexTable` · `#itemsTable` · `#dexDetailScroll` `#dexDetailContent` `#dexEncounterScroll` `#dexAcquisition` `#dexDetailBasis` `#dexSectionLabel` · `#itemDetailScroll` `#itemDetailContent` |
| 스쿼드 · 타운맵 · 셔틀 | `#squadScroll` `#squadContent` `#squadName` `#squadRulePill` `#squadCount` `#squadShuttleButton` `#squadToolButton` `#squadIconButton` `#squadSaveStatus[state]` `#squadProblemPill[ok]` `#squadNote` `#squadResources` `#squadLinkButton` `#squadEmpty` `#squadPickerList` · `#pageTitle` `#mapRegionChip` `#mapInfoTab` · `#shuttlePickButton` `#shuttleTable` `#shuttleStatus[state]` · `QLineEdit[role="memo"]` |

이름이 화면 이름으로 시작해도 다른 화면에서 다시 쓰는 것이 있다(예: `#dexSectionLabel` · `#squadEmpty` · `#squadScroll`은 타운맵 · 선택 창에서도 쓴다).

### 동적 속성 — 상태에 따라 모양 바꾸기

```cpp
m_saveStatus->setProperty("state", QStringLiteral("pending"));
m_saveStatus->style()->unpolish(m_saveStatus);   // 또는 polish만 — 속성이 바뀌었다고 스타일에 알린다
m_saveStatus->style()->polish(m_saveStatus);
```
```css
QLabel#squadSaveStatus[state="pending"] { color: @text.3; }
QLabel#squadSaveStatus[state="error"]   { color: @red; }
```
속성을 바꾸기만 하면 QSS가 다시 적용되지 않는다 — **polish를 다시 해야** 한다. 쓰는 곳: `state`(저장 상태 · 셔틀 상태), `ok`(문제 알약), `role="memo"`(카드 메모).

### 안 쓰이는 규칙 (2026-10-08)

- `QPushButton[variant="primary"|"info"|"text"|"danger"]` — 코드에서 `variant` 속성을 주는 곳이 없다
- app.qss 39행 주석은 `ShadowButton`을 `QPushButton` 하위 클래스라고 하지만 실제로는 `QAbstractButton`이다

## 5. 글꼴

`resources/fonts/`의 TTF 8개를 qrc에 넣고 실행 시 등록한다(라이선스 OFL — `THIRD_PARTY_NOTICES.md`).

| 상수(`theme.h`) | 글꼴 | 쓰임 |
|---|---|---|
| `kFamilyTitle` | Do Hyeon | 제목 · 메뉴 · 버튼 |
| `kFamilyBody` | NanumGothic(보통 · 굵게 · 아주 굵게) | 본문 |
| `kFamilyData` | NanumGothicCoding | 숫자 · 키 |
| `kFamilyPixel` | Silkscreen | 워드마크 |

`theme::font(가족, 픽셀 크기, 굵기)` — 포인트가 아니라 **픽셀 크기**(`setPixelSize`)와 `PreferNoHinting`을 써서 글자 폭이 디자인(브라우저)과 같게 나온다. 손그림 위젯은 모두 이 함수로 글꼴을 만든다.

## 6. 데이터로 정하는 스타일 (JSON)

| 파일 | 읽는 곳 | 내용 |
|---|---|---|
| `resources/theme/dexstyle.json` | `ui/theme/dexstyle` | 버전 50개의 약칭 · 바탕 · 글자색, 도감 이름, 게임 묶음 라벨, **세대별 버전 목록**(세대 버튼 줄무늬) |
| `resources/theme/itemstyle.json` | `ui/theme/itemstyle` | 아이템 분류 묶음 8개(이름 · 색 · 카테고리 · 주머니), 숨길 카테고리 |
| `resources/theme/homecards.json` | `ui/home/homecards` | 인트로 세대 카드 9장 |

모두 처음 쓸 때 한 번 읽어 함수 안 `static`에 둔다. 버전 · 세대가 늘어도 **코드가 아니라 JSON 한 줄**을 더한다(규칙을 데이터로 — CLAUDE.md §4).

## 7. 커서 · 로그

- `cursors::pointer()` · `cursors::grab()` — `:/icons/cursor/glove-*.svg`로 만든 장갑 커서(32px, 고해상도 대응). 누를 수 있는 손그림 위젯이 `Qt::PointingHandCursor` 대신 쓴다
- UI 로그 카테고리는 `pokesix.ui` 하나(`lcUi`). 테마 · 글꼴 · JSON 로드 경고가 여기로 나온다. data는 `pokesix.data`
