# 번들 글꼴

네 글꼴 모두 SIL Open Font License 1.1 — 앱에 넣어 배포할 수 있다(라이선스 파일 동봉 필수).
이 패키지에는 파일을 넣지 않았다. 아래 원본 저장소에서 TTF를 받아 `resources/fonts/`에 두고 `app.qrc`에 등록한다.

| 역할 | 글꼴 | 필요한 굵기 | 원본 |
|---|---|---|---|
| 제목 · 탭 · 버튼 | Do Hyeon (도현) | Regular | github.com/google/fonts → `ofl/dohyeon/` |
| 본문 | Nanum Gothic (나눔고딕) | Regular · Bold · ExtraBold | github.com/google/fonts → `ofl/nanumgothic/` |
| 데이터 · 숫자 | Nanum Gothic Coding (나눔고딕코딩) | Regular · Bold | github.com/google/fonts → `ofl/nanumgothiccoding/` |
| 워드마크 · 도트 숫자 | Silkscreen | Regular · Bold | github.com/google/fonts → `ofl/silkscreen/` |

- 경로는 받는 시점에 확인할 것(폴더명이 바뀌었을 수 있음). 각 폴더의 `OFL.txt`를 함께 복사.
- 등록: `QFontDatabase::addApplicationFont(":/fonts/DoHyeon-Regular.ttf")` … 반환값이 -1이면 로그.
- 대체 글꼴(로드 실패 시): 도현 → "Black Han Sans"/"Malgun Gothic", 나눔고딕 → "Apple SD Gothic Neo"/"Malgun Gothic", 나눔고딕코딩 → "D2Coding"/monospace, Silkscreen → "Courier New".
- Silkscreen은 숫자에 **Regular(400)** 를 쓴다. Bold에서 "4"가 뭉개진다. Bold는 워드마크 "POKESIX"에만.
