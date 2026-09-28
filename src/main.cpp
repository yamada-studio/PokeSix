#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QSettings / QStandardPaths 가 저장 경로를 정할 때 이 값들을 사용한다.
    //   Linux: ~/.config/YamadaStudio/ (설정), ~/.local/share/YamadaStudio/PokeSix/ (데이터)
    QApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QApplication::setApplicationName(QStringLiteral("PokeSix"));
    QApplication::setApplicationVersion(QStringLiteral(POKESIX_VERSION));

    // TODO: src/app 의 MainWindow 로 교체
    QMainWindow window;
    window.setWindowTitle(QStringLiteral("PokeSix %1").arg(QApplication::applicationVersion()));
    window.resize(1024, 720);
    window.show();

    return app.exec();
}
