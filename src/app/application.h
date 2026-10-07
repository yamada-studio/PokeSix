#pragma once

#include <QApplication>
#include <QSize>
#include <QString>
#include <QTranslator>

#include <memory>
#include <optional>

namespace com::yamada::studio {
class AppState;
class DataUpdater;
class MainWindow;
class Repository;
enum class Page;

// 앱 부트스트랩(composition root, architecture.md §2 app). 레이어들의 객체를 **만들고 잇는 유일한
// 곳**이다: 명령행 인자 → 로그 형식 → 언어 · 테마 → AppState · Repository · DataUpdater →
// MainWindow(생성자 주입). 다른 레이어는 받은 포인터만 쓰고, 아무도 app에 의존하지 않는다.
//
//   PokeSix [--language ko|en|ja]
//   PokeSix --screenshot <screen> <WxH> <out.png> [--screenshot-delay <ms>]
//           screen = intro · dex · items · map · squad · settings
//           창을 그 크기로 띄워 그 화면을 연 뒤, 애니메이션이 끝날 만큼 기다렸다가
//           QWidget::grab()으로 PNG를 쓰고 종료한다(0 = 성공). 디스플레이가 없으면
//           QT_QPA_PLATFORM=offscreen으로 돌린다. 디자인 기준
//           이미지(design/handoff-v2/images/screens/30_intro_1440.png …)와 나란히 비교하는 진단
//           도구다(로드맵 A3 · A10).
class Application
{
public:
    Application(int &argc, char **argv);
    ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    // 이벤트 루프를 돌리고 종료 코드를 돌려준다. 인자 오류면 kExitUsage(사용법은 stderr에).
    int run();

    static constexpr int kExitUsage = 2;

private:
    struct Screenshot
    {
        QString screen;
        std::optional<Page> page; // 없음 = 인트로
        QSize size;
        QString outPath;
        int delayMs = 0;
    };

    bool parseArguments(); // false = 인자 오류(메시지는 이미 출력)
    static void setupLogging();
    static void setupWindowIcon();
    void applyLanguage(const QString &code);
    void buildObjects();
    void capture(const Screenshot &shot);

    QApplication m_app; // 먼저 선언 = 먼저 생성 · 마지막에 소멸(위젯들이 먼저 사라진다)
    QTranslator m_translator;
    std::optional<Screenshot> m_screenshot;

    // 소멸 순서는 선언의 역순: 창 → 받기 → 상태 → DB. 창이 쓰는 것들이 창보다 오래 산다.
    std::unique_ptr<Repository> m_repository;
    std::unique_ptr<AppState> m_state;
    std::unique_ptr<DataUpdater> m_updater;
    std::unique_ptr<MainWindow> m_window;
};
} // namespace com::yamada::studio
