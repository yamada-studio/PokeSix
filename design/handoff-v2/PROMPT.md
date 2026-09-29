# PokeSix 디자인 v2 — Claude Code 구현 프롬프트

> 이 패키지를 저장소의 `docs/design-handoff-v2/`에 두고 "docs/design-handoff-v2/PROMPT.md를 읽고 시작해"라고 지시하세요.
> 기존 `docs/design-handoff/`(v1)는 **수정하지 않는다.** v1 문서 중 v2에서 바뀐 부분은 이 패키지의 `docs/*b_*.md`가 덮어쓴다.

---

## 너의 역할

너는 PokeSix(C++17 · Qt 6 Widgets)에 **디자인 요청 0001**의 결과를 반영한다. 세 가지다.

1. **인트로(타이틀) 화면** 신설 — v1의 엔트리 홈(SCR-01)을 대체
2. **앱 막대 4탭** — "홈" 탭 제거, 마크 + 워드마크가 인트로로 돌아가는 버튼
3. **마크 · 앱 아이콘 교체** — 육각 "여섯 칸" → 둥근 "캡슐" 마크, 전 플랫폼 아이콘 · 파비콘

v1의 색 토큰, 타이포, 먹색 2px 외곽선, 블러 없는 오프셋 그림자, 컬러 머리의 창, 노란 선택 칸 + ▶ 커서, 해요체, 대비 4.5:1, 키보드 조작 원칙은 **그대로** 유지된다.

## 1. 현황

| 항목 | 상태 |
|---|---|
| v1 핸드오프 | `docs/design-handoff/` — 도감 · 아이템 · 스쿼드 · 설정 화면, 컴포넌트, 규칙 · 테스트. 여기 규격은 계속 유효 |
| v2 디자인 | **완료.** 인트로 3크기(1440 · 1024 · 960) + 상태 보드, 4탭 앱 막대를 반영한 기존 화면 캡처 전부, 마크 시안 비교와 확정안 |
| 마크 | 시안 A 말랑 육각 · B 캡슐 · C 알 중 **B 캡슐 확정** (`images/screens/40_mark_options.png`, `41_mark_final.png`) |
| 데이터 출처 | 인트로에 "데이터: PokéAPI" 표기. PokéAPI 이용 약관의 출처 표기 요구 사항은 구현 시 확인하고 보고 |
| 코드 | 이 패키지를 만든 쪽은 저장소를 보지 못했다. v1 작업이 어디까지 됐는지 먼저 확인할 것(§3-0) |

## 2. 읽는 순서

1. `images/screens/30_intro_1440.png` → `31`(세대 메뉴 열림) → `32`(1024) → `33`(960) → `34`(상태 · 앱 막대)
2. `images/screens/40_mark_options.png`, `41_mark_final.png`
3. `docs/02b_SCREEN_SPEC_V2.md` — SCR-01 인트로(신규), SCR-00 앱 막대(수정)
4. `docs/01b_DESIGN_SHEET_V2.md` — §6 마크(교체), 추가 토큰
5. `docs/03b_ARCHITECTURE_V2.md` + `images/diagrams/*.png` — 화면 구조 변경, 첫 실행 흐름
6. `CHANGELOG.md` — v1 대비 바뀐 파일 · 토큰 전체 목록
7. `design/tokens.json`, `design/Tokens.h` — v2 갱신본 (v1 파일을 이걸로 교체)
8. `design/source/*.dc.html` — 수치의 정답. 텍스트로 읽을 것(`{{…}}` · `<sc-for>`는 디자인 도구 템플릿이라 브라우저로 그대로 렌더되지 않는다. 목록 데이터는 파일 끝 `renderVals()`에 있다)
   - 새 파일: `Intro.dc.html`, `Intro1024.dc.html`, `Intro960.dc.html`, `IntroStates.dc.html`, `MarkOptions.dc.html`, `MarkFinal.dc.html`
   - 앱 막대가 바뀐 파일: `Dex`, `Items`, `Squad`, `Settings`, `DexCompact`, `SquadCompact`
   - `Home.dc.html`, `Identity.dc.html`은 **v1 유물**(참고용). 구현하지 않는다

## 3. 작업 순서와 완료 기준

단계마다 짧게 보고하고, 사용자 확인 후 다음 단계로.

**3-0. 현황 파악** — v1의 어느 단계까지 구현됐는지(`HomePage`, `AppBar`, 아이콘 리소스, `Theme`, `--screenshot`) 확인하고 보고. 아직 없는 부분은 v1 문서대로 만들되, 이 v2 변경을 처음부터 반영한다.

**3-1. 토큰 · 아이콘 교체**
- `design/Tokens.h` · `tokens.json`을 v2로 교체(추가만 있고 삭제 없음).
- `assets/icons/`로 앱 아이콘 교체: macOS `PokeSix.icns`(Info.plist `CFBundleIconFile`), Windows `pokesix.ico`(.rc), Linux `hicolor/*/apps/pokesix.png` + `scalable`(.desktop `Icon=pokesix`), `QApplication::setWindowIcon`, 파비콘(문서 · 웹용).
- 16px는 반드시 `windows/pokesix-16-pixel.png` · `favicon/favicon-16.png`(수작업 픽셀)을 쓴다.
완료: 세 OS에서 작업표시줄 · 독 · 창 제목 아이콘이 캡슐로 나오고, 옛 육각 리소스가 저장소에 남아 있지 않다.

**3-2. 화면 구조 변경** (`docs/03b_ARCHITECTURE_V2.md §1`)
- 최상위 `QStackedWidget` = `[IntroPage, Shell]`. Shell = AppBar + 기존 페이지 스택.
- `HomePage`와 "홈" 탭 삭제. 탭 4개(도감 · 아이템 · 스쿼드 · 설정), 단축키 Ctrl+1…4.
- AppBar의 마크 버튼 → 인트로로(툴팁 "처음 화면으로").
완료: 앱 시작 → 인트로 → 메뉴 선택 → 해당 탭이 활성인 본 화면 → 마크 클릭 → 인트로.

**3-3. 인트로 화면** (`02b §SCR-01`)
- 로고, 세대 버튼 + 세대 메뉴, 메뉴 5줄(도감 · 아이템 · SixSquad · 설정 · 종료), 하단 정보 줄.
- 조작: ↑↓ 이동(끝에서 순환하지 않음), Enter 선택, 1–4 바로 가기, Esc = 종료 줄로 커서 이동(두 번째 Esc에서 종료 확인), 마우스 hover가 선택을 옮김, 누르는 동안 눌림 상태.
- 포커스 링은 키보드로 포커스가 들어왔을 때만(`Qt::TabFocusReason` / `Qt::BacktabFocusReason`).
완료: `--screenshot intro 1440x900`, `1024x768`, `960x640` 캡처가 `30`·`32`·`33`과 일치. 세대 메뉴 열림 캡처가 `31`과 일치.

**3-4. 첫 실행 상태** (`02b §SCR-01 상태`, `34_intro_states_appbar.png` ③)
- DB가 없으면 메뉴 위에 FirstRunPanel(받기 전 → 받는 중 → 실패)을 끼우고, 도감 · 아이템 · SixSquad 줄은 잠금(45% · 선택 건너뜀).
- `DataUpdater` 신호(`progress(step, total, label)`, `failed(code)`, `finished()`)에 연결.
완료: DB 파일을 지우고 실행하면 ① 상태, 받는 중 ②, 네트워크를 끊으면 ③, 끝나면 패널이 사라지고 메뉴가 풀린다.

**3-5. 기존 화면 앱 막대 갱신**
완료: `11`~`15`, `20`, `21` 캡처와 앱 막대 영역이 일치(마크 · 탭 4개). 본문은 v1과 같아야 한다 — 본문이 달라졌다면 되돌린다.

## 4. 지켜야 할 선

- **포켓볼 도안을 쓰지 않는다.** 원형 몸통 + 몸통을 가로지르는 띠 + 가운데 둥근 버튼의 조합, 그리고 그 색만 바꾼 변형은 코드 · 리소스 · 스플래시 · 로딩 표시 어디에도 넣지 않는다. 로딩 스피너를 새로 만들 일이 생기면 캡슐 마크를 쓴다(회전 금지, v1의 계단식 깜빡임 규칙).
- 캡슐 마크의 비율(80×50, 반지름 25), 기울기(−35°), 반쪽 색(빨강 뒤 · 크림 앞 얼굴)을 바꾸지 않는다. 크기별로 빼는 요소는 `01b §6-3` 표를 따른다.
- 포켓몬 캐릭터 · 공식 로고 · 공식 아트는 계속 넣지 않는다.
- 인트로에 연속 애니메이션 · 블러 · 그라데이션 배경을 넣지 않는다(사선 무늬는 QPixmap 타일).

## 5. 검증

- `--screenshot` 대상에 `intro`, `intro-gen-open`, `intro-first-run`을 추가해 기준 이미지와 나란히 비교하고 차이 목록을 보고.
- v1 테스트(`04_RULES_AND_TESTS.md`)는 계속 통과해야 한다.
- 키보드만으로 인트로 → 각 화면 → 인트로 왕복이 되는지 확인.

## 6. 막히면 사용자에게 물어볼 것

1. 종료 전에 확인 대화 상자를 띄울지 (디자인은 "두 번째 Esc 또는 종료 줄 Enter → 확인" 가정)
2. 앱 버전 문자열을 어디서 가져올지 (CMake `PROJECT_VERSION` 가정)
3. PokéAPI 출처 표기 문구를 약관에 맞게 바꿔야 하는지

## 7. 패키지 구성

```
design-handoff-v2/
├─ PROMPT.md · CHANGELOG.md
├─ docs/  01b_DESIGN_SHEET_V2.md · 02b_SCREEN_SPEC_V2.md · 03b_ARCHITECTURE_V2.md
├─ images/screens/   00 기초 · 02 컴포넌트 · 11–15 · 20–21 (4탭 앱 막대) · 30–34 인트로 · 40–41 마크
├─ images/diagrams/  architecture · screen-flow (v2), squad-widget-tree · data-model (v1과 같음)
├─ design/  tokens.json · Tokens.h (v2) · pokesix.qss · source/*.dc.html
└─ assets/  icons/ (svg · macos · windows · linux · favicon) · fonts/README.md
```
