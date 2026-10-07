#include "app/application.h"

#include "data/db/gamedatabase.h"
#include "data/repository/repository.h"
#include "data/state/appstate.h"
#include "data/update/dataupdater.h"
#include "ui/logging/logging.h"
#include "ui/shell/mainwindow.h"
#include "ui/shell/page.h"
#include "ui/theme/theme.h"

#include <QCommandLineParser>
#include <QIcon>
#include <QPixmap>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

#include <cstdio>

namespace {
using com::yamada::studio::Page;

// --screenshot 의 화면 이름 → 여는 방법. 인트로는 Page가 아니라서(앱 막대 없는 첫 화면) 비워 둔다.
struct ScreenName
{
    const char *name;
    std::optional<Page> page;
};
constexpr ScreenName kScreens[] = {
        {"intro", std::nullopt}, {"dex", Page::Dex},     {"items", Page::Items},
        {"map", Page::Map},      {"squad", Page::Squad}, {"settings", Page::Settings},
};

// 인트로의 세대 카드 부채꼴이 펼쳐지는 등장 애니메이션(CardBarrel kEnterMs 760)이 끝난 뒤 찍는다
constexpr int kDefaultScreenshotDelayMs = 1200;

void usageError(const QString &message)
{
    // GUI(WIN32) 빌드에서도 stderr는 있다 — run.bat이 파일로 받아 보여 준다
    std::fputs(qPrintable(QStringLiteral("error: %1\n").arg(message)), stderr);
}
} // namespace

namespace com::yamada::studio {
Application::Application(int &argc, char **argv)
    : m_app(argc, argv)
{
    // QSettings / QStandardPaths 가 저장 경로를 정할 때 이 값들을 사용한다.
    //   Linux: ~/.config/YamadaStudio/ (설정), ~/.local/share/YamadaStudio/PokeSix/ (데이터)
    QApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QApplication::setApplicationName(QStringLiteral("PokeSix"));
    QApplication::setApplicationVersion(QStringLiteral(POKESIX_VERSION));
    // 설정은 모든 OS에서 .ini 파일(conventions.md §8). Windows의 기본값은 레지스트리라 따로 정한다.
    //   Windows %APPDATA%\YamadaStudio\PokeSix.ini · Linux ~/.config/YamadaStudio/PokeSix.ini
    QSettings::setDefaultFormat(QSettings::IniFormat);
    setupLogging();
    setupWindowIcon();
}

Application::~Application() = default;

void Application::setupLogging()
{
    // 로그 한 줄: 시각 · 수준 한 글자 · 카테고리 · 메시지. 예) 14:02:11.503 I pokesix.ui: open page
    // 2 카테고리별 켜고 끄기는 그대로 QT_LOGGING_RULES("pokesix.*.debug=true")로 한다(run
    // 스크립트의 --log).
    qSetMessagePattern(
            QStringLiteral("%{time hh:mm:ss.zzz} "
                           "%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}"
                           "%{if-critical}E%{endif}%{if-fatal}F%{endif} "
                           "%{if-category}%{category}: %{endif}%{message}"));
}

void Application::setupWindowIcon()
{
    // OS마다 아이콘을 정하는 곳이 다르다. 코드에서 설정하는 것은 Linux뿐이다.
    //   Windows: 실행 파일에 박힌 pokesix.ico(resources/platform/windows/pokesix.rc)를
    //            Qt가 창 · 작업 표시줄 아이콘으로도 그대로 쓴다. 여기서 덮어쓰면 오히려 달라진다.
    //   macOS:   .app 번들의 PokeSix.icns(Info.plist)를 Dock · Finder가 쓴다.
    //            setWindowIcon을 부르면 실행 중 Dock 아이콘이 이 PNG로 바뀌어 버린다.
    //   Linux:   독 · 런처는 설치된 .desktop 파일의 Icon=pokesix(hicolor 테마)를 쓴다.
    //            창 제목 표시줄 · Alt+Tab · .desktop이 없을 때는 아래 창 아이콘을 쓴다.
#if defined(Q_OS_LINUX)
    // 크기별 PNG를 한 QIcon에 모아 두면 필요한 크기를 골라 쓴다.
    // 16px은 벡터 축소가 아닌 수작업 픽셀 원본(디자인 01b §6-3). 경로는 resources.qrc의 alias.
    QIcon windowIcon;
    for (const int size : {16, 32, 64, 128, 256})
        windowIcon.addFile(QStringLiteral(":/icons/window/pokesix-%1.png").arg(size),
                           QSize(size, size));
    QApplication::setWindowIcon(windowIcon);

    // 실행 중인 창을 .desktop 파일과 짝짓는 이름(Wayland의 app_id, X11의 _GTK_APPLICATION_ID).
    // resources/platform/linux/com.yamada.studio.pokesix.desktop 의 파일 이름과 같아야 한다.
    QGuiApplication::setDesktopFileName(QStringLiteral("com.yamada.studio.pokesix"));
#endif
}

bool Application::parseArguments()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
            QStringLiteral("Generation-aware Pokédex and party analyzer."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption languageOption(
            QStringLiteral("language"), QStringLiteral("Display language: ko (default), en or ja."),
            QStringLiteral("code"), QStringLiteral("ko"));
    const QCommandLineOption screenshotOption(
            QStringLiteral("screenshot"),
            QStringLiteral("Capture one screen and exit: --screenshot <screen> <WxH> <out.png>. "
                           "Screens: intro, dex, items, map, squad, settings."),
            QStringLiteral("screen"));
    const QCommandLineOption delayOption(
            QStringLiteral("screenshot-delay"),
            QStringLiteral("Milliseconds to wait before capturing (default %1).")
                    .arg(kDefaultScreenshotDelayMs),
            QStringLiteral("ms"), QString::number(kDefaultScreenshotDelayMs));
    parser.addOption(languageOption);
    parser.addOption(screenshotOption);
    parser.addOption(delayOption);
    parser.addPositionalArgument(QStringLiteral("size"),
                                 QStringLiteral("Window size for --screenshot, e.g. 1440x900."),
                                 QStringLiteral("[<WxH>"));
    parser.addPositionalArgument(QStringLiteral("out"),
                                 QStringLiteral("PNG file to write for --screenshot."),
                                 QStringLiteral("<out.png>]"));
    parser.process(m_app);

    applyLanguage(parser.value(languageOption));

    if (!parser.isSet(screenshotOption)) {
        if (!parser.positionalArguments().isEmpty()) {
            usageError(QStringLiteral("unexpected arguments: %1 (did you mean --screenshot?)")
                               .arg(parser.positionalArguments().join(QLatin1Char(' '))));
            return false;
        }
        return true;
    }

    Screenshot shot;
    shot.screen = parser.value(screenshotOption);
    const auto found
            = std::find_if(std::begin(kScreens), std::end(kScreens), [&](const ScreenName &s) {
                  return shot.screen == QLatin1String(s.name);
              });
    if (found == std::end(kScreens)) {
        usageError(QStringLiteral("unknown screen '%1' (intro, dex, items, map, squad, settings)")
                           .arg(shot.screen));
        return false;
    }
    shot.page = found->page;

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 2) {
        usageError(QStringLiteral("--screenshot needs <WxH> and <out.png>, e.g. "
                                  "--screenshot intro 1440x900 out.png"));
        return false;
    }
    static const QRegularExpression sizePattern(QStringLiteral("^(\\d{2,5})x(\\d{2,5})$"));
    const QRegularExpressionMatch size = sizePattern.match(positional.at(0));
    if (!size.hasMatch()) {
        usageError(QStringLiteral("bad size '%1' (expected <width>x<height>, e.g. 1440x900)")
                           .arg(positional.at(0)));
        return false;
    }
    shot.size = QSize(size.captured(1).toInt(), size.captured(2).toInt());
    shot.outPath = positional.at(1);
    bool delayOk = false;
    shot.delayMs = parser.value(delayOption).toInt(&delayOk);
    if (!delayOk || shot.delayMs < 0) {
        usageError(QStringLiteral("bad --screenshot-delay '%1'").arg(parser.value(delayOption)));
        return false;
    }
    m_screenshot = shot;
    return true;
}

void Application::applyLanguage(const QString &code)
{
    // --language ko|en|ja 로 고른다. 없으면 한국어(기본값). 고른 값은 저장해서 AppState가 읽는다.
    //   화면 문구: 번역 파일(:/i18n/pokesix_<언어>.qm)을 설치하면 tr("…")이 그 언어 글자를
    //   돌려준다. 소스 언어가 한국어라 한국어는 번역 파일 없이 그대로다. 위젯을 만들기 전에
    //   설치해야 생성자의 tr()부터 적용된다.
    //   게임 데이터 이름: AppState::language를 화면들이 따른다(실행 중에도 바로 바뀐다).
    const Language language = languageFromCode(code);
    AppState::saveLanguage(language);
    if (language != Language::Korean
        && m_translator.load(QStringLiteral(":/i18n/pokesix_%1.qm").arg(languageCode(language))))
        QApplication::installTranslator(&m_translator);
}

void Application::buildObjects()
{
    // 디자인 스타일: 번들 글꼴 등록 + app.qss(@token 치환) 적용. 위젯을 만들기 전에 불러야 한다.
    theme::apply(m_app);

    // 게임 데이터 DB 조회 창구. QObject가 아니라서 unique_ptr로 소유한다. 화면들은 포인터만 받는다.
    m_repository = std::make_unique<Repository>(gamedatabase::defaultPath());
    // 앱 상태(정주행 중인 세대 · 게임 · 언어). 화면들은 이 객체를 받아 *Changed를 구독한다.
    m_state = std::make_unique<AppState>();
    // 첫 실행 데이터 받기 · 변환(worker 스레드). 인트로의 FirstRunPanel이 쓴다.
    m_updater = std::make_unique<DataUpdater>();
    m_window = std::make_unique<MainWindow>(m_repository.get(), m_state.get(), m_updater.get());
}

void Application::capture(const Screenshot &shot)
{
    QPixmap pixmap = m_window->grab();
    if (pixmap.save(shot.outPath)) {
        qCInfo(lcUi) << "screenshot" << shot.screen << pixmap.size() << "->" << shot.outPath;
        m_app.exit(0);
        return;
    }
    usageError(QStringLiteral("could not write %1").arg(shot.outPath));
    m_app.exit(1);
}

int Application::run()
{
    if (!parseArguments())
        return kExitUsage;
    buildObjects();

    if (!m_screenshot) {
        m_window->show();
        return m_app.exec();
    }

    // 캡처 모드: 창을 요청 크기로 띄우고 그 화면을 연 뒤, 애니메이션이 끝날 만큼 기다렸다가 찍는다.
    // 타이머 콜백은 이벤트 루프 안에서 돌므로 레이아웃 · polish · 첫 paint가 끝난 상태다.
    const Screenshot &shot = *m_screenshot;
    m_window->resize(shot.size);
    m_window->show();
    if (shot.page)
        m_window->open(*shot.page);
    else
        m_window->showIntro();
    QTimer::singleShot(shot.delayMs, m_window.get(), [this, shot] { capture(shot); });
    return m_app.exec();
}
} // namespace com::yamada::studio
