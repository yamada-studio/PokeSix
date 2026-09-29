#include "ui/shell/mainwindow.h"
#include "ui/theme/theme.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QSettings / QStandardPaths 가 저장 경로를 정할 때 이 값들을 사용한다.
    //   Linux: ~/.config/YamadaStudio/ (설정), ~/.local/share/YamadaStudio/PokeSix/ (데이터)
    QApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QApplication::setApplicationName(QStringLiteral("PokeSix"));
    QApplication::setApplicationVersion(QStringLiteral(POKESIX_VERSION));

    // 창 아이콘(제목 표시줄 · 작업 표시줄 · Alt+Tab). 크기별 PNG를 한 QIcon에 모아 두면
    // OS가 필요한 크기를 고른다. 16px은 벡터 축소가 아닌 수작업 픽셀 원본(디자인 01b §6-3).
    //   실행 파일 아이콘은 따로다: Windows = pokesix.rc, macOS = .app 번들의 PokeSix.icns,
    //   Linux 데스크톱 런처 = 설치된 .desktop + hicolor 아이콘 (src/CMakeLists.txt의 install)
    QIcon windowIcon;
    for (const int size : {16, 32, 64, 128, 256})
        windowIcon.addFile(QStringLiteral(":/icons/window/pokesix-%1.png").arg(size),
                           QSize(size, size));
    QApplication::setWindowIcon(windowIcon);

    // Linux(Wayland · GNOME 등)는 실행 중인 창을 이 이름의 .desktop 파일과 짝지어
    // 독 · 작업 표시줄 아이콘을 정한다. resources/platform/linux/ 의 파일 이름과 같아야 한다.
    QGuiApplication::setDesktopFileName(QStringLiteral("com.yamada.studio.pokesix"));

    // 디자인 스타일: 번들 글꼴 등록 + app.qss(@token 치환) 적용. 위젯을 만들기 전에 불러야 한다.
    com::yamada::studio::theme::apply(app);

    com::yamada::studio::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
