#include "ui/shell/mainwindow.h"

#include "data/db/gamedatabase.h"
#include "data/repository/repository.h"
#include "data/update/dataupdater.h"
#include "ui/dex/dexpage.h"
#include "ui/home/homepage.h"
#include "ui/logging/logging.h"
#include "ui/shell/appbar.h"
#include "ui/widgets/searchfield.h"

#include <QApplication>
#include <QKeySequence>
#include <QLabel>
#include <QShortcut>
#include <QStackedWidget>
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
    //  ├ [0] HomePage (인트로)
    //  └ [1] shell (본 화면)
    //         └ QVBoxLayout (margins 0, spacing 0)
    //            ├ AppBar    높이 60 — 마크 · 탭 4 · 세대 버튼 · 검색
    //            └ m_pages   [도감(DexPage) · 아이템 · 스쿼드 · 설정(자리 표시)]

    // 첫 실행 데이터 받기 · 변환. 인트로가 쓰지만 MainWindow가 만들어 넘긴다(생성자 주입).
    // A3에서 앱 초기화 계층(pokesix_app)이 생기면 거기서 만들어 넘기게 된다.
    DataUpdater *dataUpdater = new DataUpdater(this);

    m_screens = new QStackedWidget;
    HomePage *home = new HomePage(dataUpdater);
    m_screens->addWidget(home);

    QWidget *shell = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(shell); // installs itself on shell
    layout->setContentsMargins(0, 0, 0, 0);       // no style default margins (~9-11px)
    layout->setSpacing(0);                        // no style default spacing (~6px)

    m_appBar = new AppBar;
    m_pages = new QStackedWidget;
    // 페이지 순서는 Page(page.h)와 같다. 도감은 D2에서 실제 화면이 되었고, 나머지는 Phase E에서
    // 하나씩 채운다. 지금은 이름만 보이는 자리 표시.
    // Repository는 게임 데이터 DB 조회 창구다. 화면들은 포인터만 받아 쓴다(생성자 주입).
    m_repository = std::make_unique<Repository>(gamedatabase::defaultPath());
    m_pages->addWidget(new DexPage(m_repository.get())); // [0] 도감
    const QString names[] = {tr("아이템"), tr("스쿼드"), tr("설정")};
    for (const QString &name : names) {
        QLabel *placeholder = new QLabel(tr("%1 — 준비 중이에요").arg(name));
        placeholder->setObjectName(QStringLiteral("pagePlaceholder"));
        placeholder->setAlignment(Qt::AlignCenter);
        m_pages->addWidget(placeholder);
    }

    layout->addWidget(m_appBar);
    layout->addWidget(m_pages);
    m_screens->addWidget(shell);

    QMainWindow::setCentralWidget(m_screens);

    // 화면 전환은 전부 여기서 한다. 인트로 · 앱 막대는 "무엇을 원한다"만 신호로 알린다.
    connect(home, &HomePage::openRequested, this, &MainWindow::open);
    connect(m_appBar, &AppBar::pageSelected, this, &MainWindow::open);
    connect(m_appBar, &AppBar::homeRequested, this, &MainWindow::showIntro);
    // 종료: 확인 대화 상자 여부는 열린 질문 9(roadmap). 지금은 바로 창을 닫는다(마지막 창 → 앱
    // 종료).
    connect(home, &HomePage::quitRequested, this, &QMainWindow::close);

    // 본 화면의 단축키. shell에 달고 WidgetWithChildrenShortcut으로 두면 본 화면이 보일 때만
    // 동작한다 (인트로에서는 1–4 키가 메뉴에 있고, 잠긴 메뉴를 단축키로 우회하지 못한다).
    const Page pages[] = {Page::Dex, Page::Items, Page::Squad, Page::Settings};
    for (int i = 0; i < 4; ++i) {
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), shell);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        const Page page = pages[i];
        connect(shortcut, &QShortcut::activated, this, [this, page] { open(page); });
    }
    QShortcut *search = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), shell);
    search->setContext(Qt::WidgetWithChildrenShortcut);
    connect(search, &QShortcut::activated, this,
            [this] { m_appBar->searchField()->setFocus(Qt::ShortcutFocusReason); });

    qCInfo(lcUi) << "MainWindow initialized";
}

MainWindow::~MainWindow() = default;

void MainWindow::open(Page page)
{
    m_pages->setCurrentIndex(static_cast<int>(page)); // 페이지 순서 = Page 순서
    m_appBar->setCurrentPage(page);
    m_screens->setCurrentIndex(MainScreen);
    qCInfo(lcUi) << "open page" << static_cast<int>(page);
}

void MainWindow::showIntro()
{
    m_screens->setCurrentIndex(IntroScreen); // HomePage::showEvent가 메뉴에 포커스를 준다
}
} // namespace com::yamada::studio
