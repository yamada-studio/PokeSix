# D5 — 첫 실행: 데이터 받기 → 변환(worker 스레드) → 인트로 잠금 해제

> 학습 루프 ① 가이드. 브랜치: `feat/d5-first-run` (Claude가 만들어 둠)
> 뼈대 파일은 Claude가 만들어 두었다. **`// TODO`만 채우면 된다.** 빈칸이 있어도 빌드된다.

## 전체 그림

```
앱 시작 ─ DataUpdater::hasData()? ─ 예 ──▶ 평소처럼 인트로
                                  └ 아니오 ─▶ 인트로에 FirstRunPanel "데이터 받기"   (CP3 · CP4)
                                               │ 누르면
                                               ▼
   UI 스레드: CsvDownloader가 CSV를 받는다(D4, 비동기)          → 진행 0–90%
   worker 스레드: CsvImporter가 SQLite로 변환한다(D1, 동기)       → 90–100%   (CP1)
   UI 스레드: 패널이 사라지고 메뉴 잠금이 풀린다                                 (CP2 · CP4)
```

| | 체크포인트 | 파일 | 배우는 것 |
|---|---|---|---|
| ▶ | **CP1 `DataUpdater`** | `src/data/db/gamedatabase.cpp`, `src/data/update/dataupdater.cpp` | **worker 스레드**(`moveToThread`), 스레드를 넘는 시그널/슬롯 |
| | CP2 메뉴 잠금 | `src/ui/home/intromenu*`, `intromenuitem*` | 상태를 가진 위젯, 선택 건너뛰기 |
| | CP3 `FirstRunPanel` | `src/ui/home/firstrunpanel*` (새 파일) | 상태 3개(받기 전 · 받는 중 · 실패)를 가진 화면 |
| | CP4 연결 | `homepage.cpp`, `mainwindow.cpp` | 시그널/슬롯으로 부품 잇기 |

CP2부터의 자세한 단계는 CP1을 마치면 이 파일에 이어서 쓴다.

---

## CP1 — `DataUpdater`

### 왜 스레드가 필요한가
`CsvImporter::run()`은 **동기 함수**다. 실제 데이터로 0.15초지만, 기술 · 습득 기술(D1b)이 들어가면 수 초가 된다.
UI 스레드에서 부르면 그동안 이벤트 루프가 멈춘다. 창을 옮길 수도, 다시 그릴 수도, 진행 막대를 움직일 수도 없다.
그래서 변환만 **worker 스레드**에서 돌린다. 받기(D4)는 원래 비동기라 UI 스레드에서 그대로 쓴다.

### Qt의 worker 패턴 (이번에 채울 코드의 전부)
```
QThread       = 스레드 하나 + 그 안에서 도는 이벤트 루프
ImportWorker  = 일을 하는 QObject. moveToThread로 그 스레드에 "산다"
신호 → 슬롯    = 두 객체가 다른 스레드에 살면 Qt가 자동으로 큐 연결로 만든다:
                인자를 복사해 받는 쪽 스레드의 큐에 넣고, 그 스레드가 슬롯을 부른다
```
ROS 2로 치면 무거운 콜백을 다른 executor 스레드에 맡기고, 결과를 토픽으로 되돌려 받는 구조다.
**뮤텍스가 필요 없다.** 두 스레드는 데이터를 공유하지 않고, 신호의 인자(복사본)로만 주고받는다.

### 0단계 — 준비된 것 (Claude가 만들어 둠)
- `src/data/db/gamedatabase.h/.cpp`: DB 경로와 "쓸 수 있는 DB인가" 확인 — **TODO A · B**
- `src/data/update/dataupdater.h`: 클래스 선언과 설명(완성)
- `src/data/update/dataupdater.cpp`: 흐름 — **TODO ① – ⑨**
- CMake: `pokesix_data`에 파일 추가, `pokesix_ui`가 `pokesix_data`를 링크
- `src/ui/shell/mainwindow.cpp`: **임시 확인 코드**(`[임시 · D5 CP1 확인용]`) — DB가 없으면 `DataUpdater`를 돌리고 진행을 로그로 찍는다. CP4에서 인트로 패널로 옮긴다

### 1단계 — `gamedatabase.cpp`

**TODO A** `defaultPath()` — `return QString();` 줄을 바꾼다
```cpp
return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/pokesix.sqlite");
```
(설정의 조직 · 앱 이름이 `YamadaStudio` / `PokeSix`라서 Linux에서는 `~/.local/share/YamadaStudio/PokeSix/pokesix.sqlite`가 된다)

**TODO B** `isUsable()` — 주석 아래에 쓴다. 할 일: 질의를 실행하고, 한 줄이 나오면 그 값을 버전과 비교한다.
- `query.exec(QStringLiteral("SELECT value FROM meta WHERE key = 'schema_version'"))` — 성공하면 `true`
- `query.next()` — 결과의 첫 줄로 간다. 줄이 있으면 `true`
- `query.value(0).toInt()` — 첫 칸의 값
- 셋이 다 맞으면 `usable = (그 값 == schema::kVersion);`

### 2단계 — `dataupdater.cpp`의 생성자 (TODO ① – ⑤)
각 TODO 주석 **바로 아래에** 한 줄씩 쓴다.

| TODO | 쓸 코드의 모양 |
|---|---|
| ① | `m_worker->moveToThread(m_thread);` |
| ② | `connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);` |
| ③ | `connect(this, &DataUpdater::importRequested, m_worker, &ImportWorker::importCsv);` |
| ④ | `connect(m_worker, &ImportWorker::importFinished, this, &DataUpdater::onImportFinished);` |
| ⑤ | `m_thread->start();` |

- ③ · ④에 연결 방식을 적지 않아도 된다. `m_worker`는 ①에서 다른 스레드로 옮겨졌으므로, Qt가 신호를 보낼 때 보고 알아서 큐로 넘긴다
- ② 이유: worker는 부모가 없어서(다른 스레드로 옮길 객체는 부모를 가질 수 없다) object tree가 지워 주지 않는다. 스레드가 끝날 때 **그 스레드 안에서** 지우게 한다

### 3단계 — 소멸자 (TODO ⑥)
```cpp
m_thread->quit(); // 이벤트 루프에 "끝내라"를 보낸다(곧바로 끝나지는 않는다)
m_thread->wait(); // 실제로 끝날 때까지 기다린다
```
이게 없으면 앱을 닫을 때 `QThread: Destroyed while thread is still running`과 함께 죽는다. 돌고 있는 스레드보다 `QThread` 객체가 먼저 지워지기 때문이다.

### 4단계 — 흐름 잇기 (TODO ⑦ · ⑧ · ⑨)
- **⑦** `onDownloadFinished()`: `emit importRequested(m_downloader->directory(), gamedatabase::defaultPath());`
- **⑧** `onImportFinished()`: `Q_UNUSED` 두 줄을 지우고
  ```cpp
  if (ok) { emit progress(100, tr("준비 완료")); emit finished(); }
  else    { emit failed(error); }
  ```
  (clang-format이 여러 줄로 펴 준다)
- **⑨** `ImportWorker::importCsv()`: `Q_UNUSED(csvDir)`와 `emit importFinished(false, …)` 두 줄을 지우고
  ```cpp
  CsvImporter importer;
  const bool ok = importer.run(csvDir, dbPath);
  emit importFinished(ok, importer.errorString());
  ```

### 5단계 — 빌드와 실행
```bash
scripts/linux/build.sh --format
rm -f ~/.local/share/YamadaStudio/PokeSix/pokesix.sqlite   # 첫 실행 상황을 만든다(없으면 그냥 넘어감)
scripts/linux/run.sh
```
터미널 기대 출력(순서대로, `데이터 받는 중` 줄은 여러 번 나온다):
```
pokesix.ui: 0 % "데이터 받는 중"
pokesix.ui: UI thread is QThread(0x…aa10, name = "Qt mainThread")
pokesix.ui: MainWindow initialized
pokesix.data: all 36 CSV files are present and verified
pokesix.ui: 90 % "데이터 정리하는 중"
pokesix.data: importing on thread QThread(0x…16e0)                              ← 다른 스레드
pokesix.data: imported PokéAPI CSV "…" into "…/.local/share/YamadaStudio/PokeSix/pokesix.sqlite"
pokesix.data: import result on thread QThread(0x…aa10, name = "Qt mainThread")  ← 다시 UI 스레드
pokesix.ui: 100 % "준비 완료"
pokesix.ui: data ready
```
**`importing on thread`의 주소가 `UI thread is`와 다르고, `import result on thread`는 다시 같으면** 스레드가 제대로 나뉜 것이다.

창을 닫고 한 번 더 실행하면 `pokesix.ui: data already available`이 나와야 한다(TODO B가 동작하는 것).

### 막히면
| 증상 | 의심할 곳 |
|---|---|
| `90 % "데이터 정리하는 중"`에서 멈춘다 | ⑦, 또는 ③(신호가 worker에 닿지 않음), 또는 ⑤(스레드의 이벤트 루프가 안 돎) |
| `importing on thread`가 UI 스레드 주소와 같다 | ① |
| 끝났는데 `data ready`가 안 나온다 | ④ 또는 ⑧ |
| 창을 닫을 때 `QThread: Destroyed while thread is still running` | ⑥ |
| 두 번째 실행에도 다시 받는다 | A · B |

끝나면 "D5 CP1 진단해줘"라고 요청한다.

### 읽을 문서
- [QThread](https://doc.qt.io/qt-6/qthread.html) — 첫 예제가 이번 패턴(Worker + moveToThread)이다
- [Threads and QObjects](https://doc.qt.io/qt-6/threads-qobject.html) — "Signals and Slots Across Threads"
