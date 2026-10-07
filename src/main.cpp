#include "data/state/appstate.h"
#include "ui/shell/mainwindow.h"
#include "ui/theme/theme.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QSettings>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QSettings / QStandardPaths 가 저장 경로를 정할 때 이 값들을 사용한다.
    //   Linux: ~/.config/YamadaStudio/ (설정), ~/.local/share/YamadaStudio/PokeSix/ (데이터)
    QApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QApplication::setApplicationName(QStringLiteral("PokeSix"));
    QApplication::setApplicationVersion(QStringLiteral(POKESIX_VERSION));
    // 설정은 모든 OS에서 .ini 파일(conventions.md §8). Windows의 기본값은 레지스트리라 따로 정한다.
    //   Windows %APPDATA%\YamadaStudio\PokeSix.ini · Linux ~/.config/YamadaStudio/PokeSix.ini
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // ── 앱 아이콘 ─────────────────────────────────────────────────────────────
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

    // ── 언어 ──────────────────────────────────────────────────────────────────
    // --language ko|en|ja 로 고른다. 없으면 한국어(기본값). 고른 값은 저장해서 AppState가 읽는다.
    //   화면 문구: 번역 파일(:/i18n/pokesix_<언어>.qm)을 설치하면 tr("…")이 그 언어 글자를
    //   돌려준다.
    //             소스 언어가 한국어라 한국어는 번역 파일 없이 그대로다. 위젯을 만들기 전에
    //             설치해야 생성자의 tr()부터 적용된다.
    //   게임 데이터 이름: AppState::language를 화면들이 따른다(실행 중에도 바로 바뀐다).
    using com::yamada::studio::AppState;
    using com::yamada::studio::Language;
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption languageOption(
            QStringLiteral("language"), QStringLiteral("Display language: ko (default), en or ja."),
            QStringLiteral("code"), QStringLiteral("ko"));
    parser.addOption(languageOption);
    parser.process(app);
    const Language language = com::yamada::studio::languageFromCode(parser.value(languageOption));
    AppState::saveLanguage(language);
    QTranslator translator;
    if (language != Language::Korean
        && translator.load(QStringLiteral(":/i18n/pokesix_%1.qm")
                                   .arg(com::yamada::studio::languageCode(language))))
        QApplication::installTranslator(&translator);

    // 디자인 스타일: 번들 글꼴 등록 + app.qss(@token 치환) 적용. 위젯을 만들기 전에 불러야 한다.
    com::yamada::studio::theme::apply(app);

    com::yamada::studio::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
