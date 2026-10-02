#include "ui/shell/mainwindow.h"

#include "data/db/gamedatabase.h"
#include "data/repository/repository.h"
#include "data/state/appstate.h"
#include "data/update/dataupdater.h"
#include "ui/dex/dexpage.h"
#include "ui/home/homepage.h"
#include "ui/items/itemspage.h"
#include "ui/logging/logging.h"
#include "ui/shell/appbar.h"
#include "ui/squad/squadpage.h"
#include "ui/widgets/generationbutton.h"
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

    // m_screens (central)
    //  ├ [0] HomePage (인트로)
    //  └ [1] shell (본 화면)
    //         └ QVBoxLayout (margins 0, spacing 0)
    //            ├ AppBar    높이 60 — 마크 · 탭 4 · 세대 버튼 · 검색
    //            └ m_pages   [도감(DexPage) · 아이템(ItemsPage) · 스쿼드 · 설정(자리 표시)]

    // 첫 실행 데이터 받기 · 변환. 인트로가 쓰지만 MainWindow가 만들어 넘긴다(생성자 주입).
    // A3에서 앱 초기화 계층(pokesix_app)이 생기면 거기서 만들어 넘기게 된다.
    DataUpdater *dataUpdater = new DataUpdater(this);

    m_screens = new QStackedWidget;
    // 앱 상태(지금은 정주행 중인 세대). 화면들은 이 객체를 받아 generationChanged를 구독한다.
    m_state = new AppState(this);

    HomePage *home = new HomePage(dataUpdater, m_state);
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
    m_dexPage = new DexPage(m_repository.get(), m_state);
    m_pages->addWidget(m_dexPage);                                  // [0] 도감
    m_pages->addWidget(new ItemsPage(m_repository.get(), m_state)); // [1] 아이템
    SquadPage *squad = new SquadPage(m_repository.get(), m_state);
    m_pages->addWidget(squad); // [2] 스쿼드
    // 데이터 받기가 끝나 DB 파일이 바뀌었다 → 옛 연결을 닫고(다음 조회가 새 파일을 연다) 앱 시작
    // 때부터 살아 있던 스쿼드를 새 데이터로 다시 읽는다
    connect(dataUpdater, &DataUpdater::finished, this, [this, squad] {
        m_repository->close();
        squad->reloadData();
    });
    // 슬롯 메뉴 "도감에서 보기" → 도감 탭으로 옮겨 그 포켓몬의 상세를 연다(스쿼드의 게임 기준 —
    // 도감에서 다른 게임을 보고 있었어도)
    connect(squad, &SquadPage::dexRequested, this,
            [this](int pokemonId, const QString &versionGroup) {
                open(Page::Dex);
                m_dexPage->openPokemon(pokemonId, versionGroup);
            });
    QLabel *placeholder = new QLabel(tr("%1 — 준비 중이에요").arg(tr("설정")));
    placeholder->setObjectName(QStringLiteral("pagePlaceholder"));
    placeholder->setAlignment(Qt::AlignCenter);
    m_pages->addWidget(placeholder); // [3] 설정

    layout->addWidget(m_appBar);
    layout->addWidget(m_pages);
    m_screens->addWidget(shell);

    QMainWindow::setCentralWidget(m_screens);
    // 기본 크기가 레이아웃 최소(minimumSizeHint)보다 작으면 창이 알아서 최소로 커진다. 최소값은
    // 글꼴 폭에 따라 OS마다 조금 다르고, QSS · 글꼴이 입혀지는(polish) show 시점에 확정된다 —
    // 그래서 여기서 계산하지 않고 창에 맡긴다.
    QMainWindow::resize(kDefaultSize);

    // 화면 전환은 전부 여기서 한다. 인트로 · 앱 막대는 "무엇을 원한다"만 신호로 알린다.
    connect(home, &HomePage::openRequested, this, &MainWindow::open);
    connect(m_appBar, &AppBar::pageSelected, this, &MainWindow::open);
    connect(m_appBar, &AppBar::homeRequested, this, &MainWindow::showIntro);
    // 종료: 확인 대화 상자 여부는 열린 질문 9(roadmap). 지금은 바로 창을 닫는다(마지막 창 → 앱
    // 종료).
    connect(home, &HomePage::quitRequested, this, &QMainWindow::close);

    // 앱 막대의 세대 버튼을 AppState에 잇는다. 버튼은 "고름"만 알리고(generationSelected), 바뀐
    // 값은 AppState가 generationChanged로 모두에게 돌려준다. 인트로의 세대 카드(CardFan)는
    // AppState를 직접 받아 스스로 잇는다 → 어느 쪽에서 바꿔도 같은 세대를 보인다.
    GenerationButton *generationButton = m_appBar->generationButton();
    generationButton->setGenerationRange(AppState::kMinGeneration, AppState::kMaxGeneration);
    generationButton->setGeneration(m_state->generation());
    connect(generationButton, &GenerationButton::generationSelected, m_state,
            &AppState::setGeneration);
    connect(m_state, &AppState::generationChanged, generationButton,
            &GenerationButton::setGeneration);

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
    // 이미 도감에 있을 때 도감 탭을 다시 누르면(상세를 보는 중이면) 목록으로 돌아간다
    if (page == Page::Dex && m_screens->currentIndex() == MainScreen
        && m_pages->currentIndex() == static_cast<int>(Page::Dex))
        m_dexPage->showList();
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
