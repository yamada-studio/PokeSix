# D1 — CSV → SQLite: 스키마 · CSV 파서 · CsvImporter

> 학습 루프 ① 가이드. 코드는 직접 작성하고, 끝나면 "D1 진단해줘"라고 요청한다.
> 브랜치: `feat/d1-schema-importer` (Claude가 만들어 둠)
>
> 역할 분담([roadmap Phase D](../roadmap.md)): 받기(D4 `CsvDownloader`)는 Claude가 만들었다. 여기부터는 직접 짠다.
> 결정의 근거: [ADR 0011](../decisions/0011-data-from-pinned-pokeapi-csv.md)

## 목표

`CsvDownloader`가 받아 둔 PokéAPI CSV를 읽어서, 앱이 쓸 **세대 구간 SQLite**(`gen_from` / `gen_to`)로 바꾼다.

**눈에 보이는 결과**
- `pokesix-import-csv <CSV 폴더> <DB 파일>` 한 번이면 DB가 생긴다
- 그 DB에 이렇게 물으면 맞는 답이 나온다
  - "5세대의 삐삐(35) 타입" → 노말, "6세대" → 페어리
  - "1세대에서 고스트 → 에스퍼" → ×0, "2세대부터" → ×2
  - "5세대에서 강철이 받는 고스트" → ×½, "6세대부터" → ×1
- `ctest`에 CSV 파서 · 변환기 테스트가 추가되어 통과한다(인터넷 없이, 작은 시드 CSV로)

---

## 0. 준비: 실제 CSV 받아 보기

```bash
cmake --build --preset linux-debug
build/linux-debug/tools/fetchcsv/pokesix-fetch-csv          # 약 10초, 35개 · 12.5 MB
ls ~/.cache/YamadaStudio/PokeSix/pokeapi-csv/168b1e8*/
```
받은 파일을 직접 열어 보는 게 이번 단계의 출발점이다. 받을 파일 목록과 각 파일의 용도는 [src/data/update/csvsource.h](../../src/data/update/csvsource.h)에 있다.

## 1. 알아야 할 개념

### 1-1. CSV 형식 (RFC 4180)

대부분의 줄은 쉼표로 자르면 끝나지만, 그렇지 않은 경우가 실제로 있다.
```
719,9,"10,000,000 Volt Thunderbolt"      ← move_names.csv: 따옴표 안의 쉼표는 구분자가 아니다
1,bulbasaur,1,,1,5,…                    ← pokemon_species.csv: 빈 칸 = 값 없음(진화 전 없음)
```
규칙:
- 필드가 `"`로 시작하면 다음 `"`까지가 한 필드다. 그 안에서 `""`는 따옴표 한 개다. 따옴표 안에는 쉼표나 줄바꿈이 들어갈 수 있다
- 빈 필드는 **SQL NULL**로 넣는다. 빈 문자열이나 0이 아니다
- 첫 줄은 열 이름이다. **열 번호가 아니라 이름으로** 값을 찾자. PokéAPI가 열을 추가해도 코드가 깨지지 않는다
- 인코딩은 UTF-8이다(`이상해씨`, `フシギダネ`)

`QString::split(',')`로는 첫 번째 예를 처리할 수 없다. 글자를 하나씩 읽는 작은 상태 기계를 직접 만든다. "따옴표 안인가 / 밖인가"라는 상태 하나면 충분하다.
```cpp
// 모양만: 한 줄(= 한 레코드)을 필드 목록으로
class CsvReader
{
public:
    explicit CsvReader(QIODevice *device);            // QFile을 열어서 넘긴다
    bool readRecord(QStringList &fields);              // 다음 레코드. 끝이면 false
    // 선택: 빈 필드를 "값 없음"으로 구별해 돌려주는 방법도 생각해 보자
};
```

### 1-2. QtSql 기초

```cpp
QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
db.setDatabaseName(path);
db.open();                                   // 반환값 확인
QSqlQuery query(db);
query.prepare(QStringLiteral("INSERT INTO type_chart VALUES (?, ?, ?, ?, ?)"));
query.addBindValue(…);                       // 값 바인딩 — 문자열을 SQL에 이어 붙이지 않는다
if (!query.exec()) qCWarning(lcData) << query.lastError().text();
```
- **연결 이름(connectionName)**: `QSqlDatabase`는 이름으로 관리되는 전역 연결이다. 연결 하나는 **그것을 만든 스레드에서만** 쓴다. D5에서 변환을 worker 스레드로 옮길 때 이 규칙이 중요해진다. 지금부터 이름을 명시하는 습관을 들이자(예: `"pokesix.import"`)
- **prepare + bind**: 같은 문장을 수천 번 실행할 때 파싱을 한 번만 하고, 따옴표나 SQL 주입 걱정이 없다
- **트랜잭션**: `db.transaction()` … `db.commit()`. SQLite는 트랜잭션 없이 INSERT할 때마다 디스크에 확정한다. 트랜잭션 하나로 묶으면 수천 배 빨라진다. 실패하면 `rollback()`
- **반환값 확인**: `exec()`, `open()`, `commit()`은 전부 실패를 반환값으로 알린다. 예외가 아니다([architecture §7](../architecture.md#7-에러-처리))
- SQLite 드라이버가 있는지는 이미 `ctest`의 `env.qt_runtime`이 확인한다

### 1-3. 설계 과제: `*_past` → `gen_from` / `gen_to`

PokéAPI는 "현재 값" 표와 "옛 값" 표(`*_past`)를 따로 둔다. `*_past`의 한 줄은 **"`generation_id` 세대까지는 이 값이었다"**는 뜻이다.
```
pokemon_types.csv        35,18,1          삐삐의 지금 타입 = 페어리(18)
pokemon_types_past.csv   35,5,1,1         5세대까지는 노말(1)

type_efficacy.csv        8,14,200         고스트(8) → 에스퍼(14) 지금 ×2
type_efficacy_past.csv   8,14,0,1         1세대까지는 ×0
                         8,9,50,5         고스트 → 강철(9) 5세대까지는 ×½ (지금은 ×1)
```
우리 스키마(설계서 [03 §5](../../design/handoff-v1/docs/03_ARCHITECTURE.md#5-데이터))는 **구간**으로 저장한다.
```
pokemon_id  slot  type_id  gen_from  gen_to
35          1     1        1         5          ← past에서
35          1     18       6         NULL       ← 현재 값: 마지막 past 세대 + 1부터
```
세대 g의 값: `WHERE gen_from <= :g AND (gen_to IS NULL OR gen_to >= :g)`.

직접 정해야 할 것:
- 한 키(예: 포켓몬 35)에 past 줄이 여러 세대에 걸쳐 있으면 구간을 어떻게 이어 붙일까? 정렬 기준은?
- `pokemon_types`는 값이 한 칸이 아니라 **슬롯 1 · 2의 묶음**이다. 한 세대의 past 줄 여러 개를 한 덩어리로 봐야 한다
- `gen_from`의 하한은 1이 맞을까, 그 포켓몬이 **처음 나온 세대**(`pokemon_species.generation_id`)가 맞을까?
  1이어도 질의 결과는 같다. 다만 "없던 세대"를 구분할 수 있는 쪽이 도감 화면에 유리하다. 어느 쪽이든 이유를 한 줄 적자
- 이 변환은 **순수 로직**이다. SQL 없이 "past 목록 + 현재 값 → 구간 목록" 함수로 떼어 내면 테스트하기 쉽다. 그 함수를 core에 둘지 data에 둘지도 판단해 보자(힌트: Qt 타입을 쓰는가?)

## 2. 만들 것

| 파일 | 내용 |
|---|---|
| `src/data/update/csvreader.h/.cpp` | 1-1의 파서 |
| `src/data/db/schema.h` | `CREATE TABLE …` 문장들. C++ raw string(`R"(…)"`)으로 둔다. 정적 라이브러리에 qrc를 넣지 않는 규칙([conventions §9](../conventions.md#9-리소스-qrc)) 때문이다 |
| `src/data/update/csvimporter.h/.cpp` | `bool import(const QString &csvDir, const QString &dbPath, QString *error)` 정도. 표마다 "CSV 열기 → 레코드 읽기 → 바인딩 → INSERT" |
| `tests/fixtures/pokeapi-csv/*.csv` | 작은 시드: 실제 파일에서 **머리줄 + 필요한 몇 줄**만 잘라 둔다(삐삐 35 · 픽시 36, 한카리아스 445, 고스트 · 에스퍼 · 강철 상성 줄 …) |
| `tests/data/csvreader_test.cpp`, `csvimporter_test.cpp` | GoogleTest. 변환기 테스트는 `QTemporaryDir`에 DB를 만들고 질의한다 |
| `tools/importcsv/` | `pokesix-import-csv <CSV 폴더> <DB>` — `tools/fetchcsv`와 같은 모양 |

`src/data/CMakeLists.txt`, `tests/data/CMakeLists.txt`, `tools/CMakeLists.txt`에 새 파일과 타깃을 추가한다.

## 3. 작업 순서 (체크포인트)

1. **CP1 — CsvReader**: 파서와 테스트
   - 테스트 거리: 일반 줄, 빈 필드, `"10,000,000 Volt Thunderbolt"`, `""` 이스케이프, 따옴표 안의 줄바꿈, 마지막 줄에 줄바꿈이 없을 때, `\r\n`
2. **CP2 — 타입과 상성**: 스키마(`types`, `type_chart`) + `types.csv` · `type_efficacy(_past).csv` 변환 + 1-3의 구간 변환
   - 완료: 목표의 상성 세 질문이 테스트로 통과한다
3. **CP3 — 종 · 이름 · 포켓몬 · 타입 · 종족값**: `species`(이름 ko / en / ja 열), `pokemon`, `pokemon_types`, `pokemon_stats`(과거 포함)
   - 이름: `pokemon_species_names.csv`의 `local_language_id` — 3 = ko, 9 = en, 11 = ja(한자 · 가나 표기). `languages.csv`에서 직접 확인
   - 완료: 삐삐 타입 두 질문 + 실제 데이터로 종 1025 · 포켓몬 1351
4. **CP4 — 도구와 실제 데이터**: `pokesix-import-csv`로 실제 CSV 전체(CP2 · CP3 범위)를 변환해 보고, 걸린 시간을 잰다
   - 트랜잭션을 뺐을 때와 넣었을 때를 비교해 본다(실험 1)

기술 · 습득 기술(63만 줄) · 아이템은 D1의 다음 체크포인트로 이어서 한다. 규모가 커서 성능이 새 주제가 된다.

## 4. 실험 (진단 때 결과를 알려 줄 것)

1. **트랜잭션**: CP4에서 트랜잭션 없이 / 있이 걸린 시간
2. **prepare 재사용**: INSERT마다 `QSqlQuery`를 새로 만들어 `prepare`할 때와, 하나를 만들어 두고 바인딩만 바꿀 때
3. **NULL**: 빈 필드를 빈 문자열로 넣었을 때 `WHERE evolves_from IS NULL`이 어떻게 달라지는가
4. **열 이름으로 찾기**: 시드 CSV의 열 순서를 바꿔도 테스트가 통과하는가

## 5. 완료 조건

- [ ] CP1–CP4, 목표의 질의가 전부 맞다
- [ ] 모든 `exec()` · `open()` · `commit()`의 반환값을 확인하고, 실패하면 `lcData`로 이유를 남기고 `false`를 돌려준다(예외 금지)
- [ ] DB 연결은 이름을 붙여 만들고, 다 쓰면 `QSqlDatabase::removeDatabase`로 정리한다
- [ ] 문자열을 이어 붙여 SQL을 만들지 않는다(값은 전부 바인딩)
- [ ] data 레이어가 Qt Widgets/Gui를 include하지 않는다
- [ ] 빌드 경고 0, `clang-format` 통과, `ctest` 통과(인터넷 없이)
- [ ] 실험 1–4를 설명할 수 있다

## 6. 막히면 먼저 볼 것

| 증상 | 의심할 곳 |
|---|---|
| `QSqlDatabasePrivate::addDatabase: duplicate connection name` | 같은 이름으로 두 번 addDatabase. 정리(removeDatabase)를 했는가 |
| `QSqlDatabasePrivate::removeDatabase: connection … is still in use` | 그 연결로 만든 `QSqlQuery` · `QSqlDatabase` 객체가 아직 살아 있다. 범위(scope)를 먼저 닫는다 |
| `Driver not loaded` | `ctest -R env` — sqldrivers 플러그인 |
| 변환이 몇 분씩 걸린다 | 트랜잭션(1-2) |
| 한글이 깨진다 | 파일을 UTF-8로 읽는가 (`QString::fromUtf8`) |
| 테스트가 가끔 실패한다 | 테스트끼리 DB 파일이나 연결 이름을 공유하지 않는가 (`QTemporaryDir`, 테스트마다 다른 이름) |

## 7. 읽을 문서

- [SQL Database Drivers — QSQLITE](https://doc.qt.io/qt-6/sql-driver.html#qsqlite)
- [QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html) — "Threads" 절과 connectionName
- [QSqlQuery](https://doc.qt.io/qt-6/qsqlquery.html) — "Approaches to Binding Values"
- [RFC 4180](https://www.rfc-editor.org/rfc/rfc4180) — CSV 규칙(짧다)
- [SQLite: INSERT가 느린 이유](https://www.sqlite.org/faq.html#q19) — FAQ 19
