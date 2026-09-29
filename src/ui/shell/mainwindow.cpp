#include "ui/shell/mainwindow.h"

#include "data/update/dataupdater.h"
#include "ui/home/homepage.h"
#include "ui/logging/logging.h"
#include "ui/shell/appbar.h"

#include <QApplication>
#include <QStackedWidget>
#include <QThread>
#include <QVBoxLayout>

namespace com::yamada::studio {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QMainWindow::setWindowTitle(
            QStringLiteral("PokeSix %1").arg(QApplication::applicationVersion()));
    QMainWindow::setMinimumSize(960, 640);
    QMainWindow::resize(1440, 900);

    // m_screens (central)
    //  ├ [0] HomePage
    //  └ [1] shell
    //         └ QVBoxLayout (margins 0, spacing 0)
    //            ├ AppBar    fixed 60
    //            └ m_pages
    //               └ (placeholder) QWidget
    m_screens = new QStackedWidget;
    HomePage *home = new HomePage;
    m_screens->addWidget(home);

    QWidget *shell = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(shell); // installs itself on shell
    layout->setContentsMargins(0, 0, 0, 0);       // no style default margins (~9-11px)
    layout->setSpacing(0);                        // no style default spacing (~6px)

    m_pages = new QStackedWidget;
    m_pages->addWidget(new QWidget);

    layout->addWidget(new AppBar);
    layout->addWidget(m_pages);

    m_screens->addWidget(shell);

    QMainWindow::setCentralWidget(m_screens);

    // 인트로 메뉴의 요청을 받는다. HomePage는 "무엇을 원한다"만 알리고, 화면 전환은 여기서 한다.
    // TODO(A9): 앱 막대(마크 → 인트로로 돌아오기)가 생기면 m_screens를 [1]로 바꾸고 해당 페이지를
    // 연다.
    //           지금은 돌아올 길이 없어서 로그만 남긴다.
    connect(home, &HomePage::openRequested, this,
            [](Page page) { qCInfo(lcUi) << "open requested: page" << static_cast<int>(page); });
    // 종료: 확인 대화 상자 여부는 열린 질문 9(roadmap). 지금은 바로 창을 닫는다(마지막 창 → 앱
    // 종료).
    connect(home, &HomePage::quitRequested, this, &QMainWindow::close);

    // [임시 · D5 CP1 확인용] DB가 없으면 받아서 변환한다. CP4에서 인트로의 FirstRunPanel로 옮긴다.
    if (!DataUpdater::hasData()) {
        DataUpdater *updater = new DataUpdater(this);
        connect(updater, &DataUpdater::progress, this,
                [](int percent, const QString &label) { qCInfo(lcUi) << percent << "%" << label; });
        connect(updater, &DataUpdater::finished, this, [] { qCInfo(lcUi) << "data ready"; });
        connect(updater, &DataUpdater::failed, this,
                [](const QString &message) { qCWarning(lcUi) << "data failed:" << message; });
        updater->start();
    } else {
        qCInfo(lcUi) << "data already available";
    }
    qCInfo(lcUi) << "UI thread is" << QThread::currentThread();

    qCInfo(lcUi) << "MainWindow initialized";
}
} // namespace com::yamada::studio
