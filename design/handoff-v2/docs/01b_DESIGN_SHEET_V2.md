# 01b · 디자인 시트 v2 (바뀐 부분만)

v1 `01_DESIGN_SHEET.md`의 **§6 아이콘 · 마크를 이 문서의 §6으로 교체**하고, §1에 아래 토큰을 추가한다. 나머지는 그대로.

## §1 추가 토큰

| 토큰 | 값 | 용도 |
|---|---|---|
| `capsule.red` | `#D8322B` | 캡슐 뒤 반쪽 (= `red`) |
| `capsule.cream` | `#FFF8EC` | 캡슐 앞 반쪽 (얼굴 쪽). 순백 아님 |
| `capsule.blush` | `#F4A3A0` | 볼 터치 (128px 이상) |
| `capsule.gloss` | `#FFFFFF` 60% | 빨강 반쪽 광택 막대 |
| `plate.macTop` → `plate.macBottom` | `#3E7FD0` → `#1D4E8F` | macOS 아이콘 플레이트 세로 그라데이션 (아이콘 전용 — 앱 UI에는 그라데이션 금지 유지) |
| `plate.linuxFace` / `plate.linuxBase` | `#FFFDF8` / `#B01F19` | Linux 아이콘 판 / 두꺼운 밑면 |
| `intro.stripe` | `#EFEADF` | 인트로 사선 무늬 두 번째 색 |
| `intro.titleShadow` | `#F2C12E` | 인트로 워드마크 오프셋 그림자 |
| `menu.pressed` | `#F2C12E` | 인트로 메뉴 눌림 바탕 |

크기 토큰 추가: `introMenuWidth 520` · `introMenuRow 58 / 52 / 48` · `introMark 136` · `introWordmark 80` · `appBarMark 36` · `appBarTabs 4`. 전체는 `design/tokens.json`.

## §6 마크 "캡슐" (v1 "여섯 칸" 육각을 대체)

기준 이미지: `images/screens/41_mark_final.png` · 원본: `design/source/MarkFinal.dc.html` · 비교: `40_mark_options.png`

### 6-1. 형태 (viewBox 100 × 100)

| 요소 | 값 |
|---|---|
| 몸통 | 캡슐 `x 10 · y 25 · 80 × 50 · 반지름 25` (두 반원 중심 (35,50) · (65,50)) |
| 반쪽 | 이음선 x = 50. 왼쪽 = `capsule.red`, 오른쪽 = `capsule.cream` |
| 기울기 | 중심 (50,50) 기준 **−35°** (오른쪽 위로) |
| 배율 | `k = 48.5 / (37.3 + max(먹선, 스티커테) / 2)` — 37.3은 −35° 회전 시 캡슐의 가로 반폭 |
| 광택 | 빨강 반쪽 `x 19 · y 31 · 18 × 6 · r 3` 흰색 60% |
| 눈 | 타원 중심 (61,47) · (77,47), 반지름은 크기 표 참고, 눈빛 점 r 1.4 at (+1.4, −1.8) |
| 입 | `M65 55.5 Q69 60 73 55.5` 먹선 2.4 · 둥근 끝 |
| 볼 터치 | 타원 (56.5,55.5) · (81.5,55.5) · 3.6 × 2.2 |
| 그리는 순서 | (스티커 테) → 빨강 반쪽 → 크림 반쪽 → 광택 → 이음선 → 외곽선 → 눈 → 눈빛 → 입 → 볼 |

**피하는 것**: 원형 몸통, 몸통을 가로지르는 띠, 가운데 둥근 버튼 — 이 셋의 조합과 그 색만 바꾼 변형. 마크를 똑바로 세우거나(0°) 캡슐 비율을 1:1에 가깝게 만들지 않는다.

### 6-2. 변형

| 변형 | 파일 | 규칙 |
|---|---|---|
| 기본 | `svg/pokesix-mark.svg` | 종이 · 흰 바탕 |
| 빨강 바탕용 | `svg/pokesix-mark-on-red.svg` | 흰 스티커 테(너비 = 먹선 + 9, 외곽선 아래에 먼저 그림). 앱 막대 36 · 스플래시 |
| 작은 크기 | `svg/pokesix-mark-small.svg` | 32 규칙 (광택 · 입 · 볼 없음, 먹선 7) |
| 단색 | `svg/pokesix-mark-mono.svg` | 빨강 면 = 먹색, 크림 = 흰색. 인쇄 · 음각용 |

### 6-3. 크기별 규칙

| 크기 | 먹선 | 이음선 | 눈 (rx × ry) | 광택 · 눈빛 | 입 | 볼 |
|---|---|---|---|---|---|---|
| 16 | — | — | 픽셀 2점 | — | — | — |
| 32 | 7 | 6 | 4.8 × 5.4 | — | — | — |
| 64 | 5 | 4.5 | 4 × 4.6 | ○ | ○ | — |
| 128+ | 4 | 3.5 | 3.6 × 4.2 | ○ | ○ | ○ |

16px은 벡터 대신 **수작업 픽셀 원본**(`favicon/favicon-16.png`, `windows/pokesix-16-pixel.png`, 16×16 비트맵은 `MarkFinal.dc.html`의 `rows` 배열).

### 6-4. 플랫폼 아이콘 (`assets/icons/`)

| 플랫폼 | 모양 | 파일 |
|---|---|---|
| macOS | 파랑 둥근 사각 플레이트(824/1024 격자, 반경 22.5%) + 스티커 캡슐(배율 0.64, 32 이하 0.72 · 16 0.78) | `macos/PokeSix.icns`, `macos/PokeSix.iconset/`, `macos/pokesix-*.png` |
| Windows | 플레이트 없음, 스티커 캡슐 | `windows/pokesix.ico`(16 · 32 · 48 · 64 · 128 · 256, 16은 픽셀 원본), `windows/pokesix-*.png` |
| Linux | 크림 판 + 두꺼운 빨강 밑면, 스티커 없음 | `linux/hicolor/{16…1024}x…/apps/pokesix.png`, `linux/hicolor/scalable/apps/pokesix.svg` |
| 파비콘 | 기본 마크 | `favicon/favicon.ico`(16 · 32 · 48), `favicon-16/32/48.png` |

### 6-5. 워드마크 조합

- 가로형: 마크 높이 = 워드마크 글자 높이 × 1.4, 간격 14. 종이 바탕에선 워드마크에 노랑 오프셋 그림자(3px).
- 앱 막대: 스티커 마크 36 + "POKESIX" Silkscreen 700 20 (SIX = `yellow.soft`).
- 인트로: 마크 136 위, 워드마크 80 아래(그림자 5px). 960 폭에선 마크 72 + 워드마크 50을 가로로.
