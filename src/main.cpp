#include "ui/shell/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QSettings / QStandardPaths 가 저장 경로를 정할 때 이 값들을 사용한다.
    //   Linux: ~/.config/YamadaStudio/ (설정), ~/.local/share/YamadaStudio/PokeSix/ (데이터)
    QApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QApplication::setApplicationName(QStringLiteral("PokeSix"));
    QApplication::setApplicationVersion(QStringLiteral(POKESIX_VERSION));

    com::yamada::studio::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
