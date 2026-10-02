#include "ui/home/intromenu.h"

#include "ui/home/intromenuitem.h"

#include <QKeyEvent>
#include <QVBoxLayout>

namespace {
constexpr int kItemSpacing = 10; // 카드 버튼 사이
constexpr int kItemHeight = 62;  // 카드 버튼(그림자 포함)
// 데이터가 있어야 쓸 수 있는 메뉴 줄(스쿼드 · 도감 백과 · 아이템 백과). 규칙을 if 대신 표로 둔다.
constexpr int kNeedsData[] = {0, 1, 2};

// 아이콘은 앱 막대 탭과 같은 그림(24 격자, 선 2.2) — 배지 색은 IntroMenuItem이 입힌다
constexpr const char *kSquadSvg
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"><path d="M12 3l7.8 4.5v9L12 21l-7.8-4.5v-9z"/><path d="M12 3v18M4.2 7.5l15.6 9M19.8 7.5l-15.6 9"/></svg>)";
constexpr const char *kDexSvg
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><rect x="4" y="4" width="16" height="16" rx="2"/><path d="M4 9h16M9 9v11"/></svg>)";
constexpr const char *kItemsSvg
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"><path d="M5 8h14l-1.2 12H6.2z"/><path d="M9 8V6.5a3 3 0 0 1 6 0V8"/></svg>)";
} // namespace

namespace com::yamada::studio {
IntroMenu::IntroMenu(QWidget *parent)
    : QWidget(parent)
{
    // 키보드 이벤트를 받으려면 포커스를 받을 수 있어야 한다. 줄(IntroMenuItem)들은 NoFocus.
    QWidget::setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kItemSpacing);

    struct Entry
    {
        IntroMenuItem::Kind kind;
        QString name;
        QString description;
        const char *svg;
    };
    const Entry entries[] = {
            // 주 행동(스쿼드)만 빨강 카드. 단축키 1–3은 keyPressEvent가 계속 받는다(칸은 안 그린다)
            {IntroMenuItem::Kind::Primary, tr("스쿼드"), tr("여섯 자리 파티 편성과 타입 분석"),
             kSquadSvg},
            {IntroMenuItem::Kind::Entry, tr("도감 백과"), tr("종족값 · 타입 상성 · 습득 기술"),
             kDexSvg},
            {IntroMenuItem::Kind::Entry, tr("아이템 백과"),
             tr("회복 · 기술머신 · 진화 · 배틀 · 기타"), kItemsSvg},
    };

    for (const Entry &entry : entries) {
        IntroMenuItem *item = new IntroMenuItem(entry.kind);
        item->setText(entry.name);
        item->setDescription(entry.description);
        item->setIconSvg(QString::fromLatin1(entry.svg));
        item->setFixedHeight(kItemHeight);
        layout->addWidget(item);
        m_items.push_back(item);
    }

    // 시그널/슬롯: 줄이 보낸 신호를 메뉴의 동작으로 잇는다.
    //   hovered → 선택 이동,  clicked → 실행
    // 람다가 index를 값으로 캡처하므로 줄마다 자기 번호를 기억한다.
    // connect의 세 번째 인자(this)는 "수신자" — this가 사라지면 연결도 자동으로 끊긴다.
    for (int index = 0; index < static_cast<int>(m_items.size()); ++index) {
        IntroMenuItem *item = m_items[index];
        connect(item, &IntroMenuItem::hovered, this, [this, index] { setCurrentIndex(index); });
        connect(item, &IntroMenuItem::clicked, this, [this, index] { emit activated(index); });
    }

    setCurrentIndex(0); // 처음 선택 = 도감 백과
}

void IntroMenu::setCurrentIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;
    // 잠긴 줄이면 고르지 않고 돌아간다: m_items[index]->isEnabled()
    //        (마우스 hover는 비활성 줄에서도 hovered()를 보내므로, 여기서 막아야 선택이 옮겨 가지
    //        않는다)
    if (!m_items[index]->isEnabled())
        return;
    m_current = index;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
        m_items[i]->setSelected(i == m_current);
}

void IntroMenu::setDataLocked(bool locked)
{
    // kNeedsData의 줄마다 setEnabled(!locked)
    for (const int index : kNeedsData)
        m_items[index]->setEnabled(!locked);

    // 잠갔으면: 지금 선택이 잠긴 줄이면 nextEnabled(m_current, +1)로 옮긴다
    //        풀었으면: setCurrentIndex(0) (디자인: 완료되면 선택은 도감 백과)
    if (locked) {
        if (!m_items[m_current]->isEnabled())
            setCurrentIndex(nextEnabled(m_current, +1));
    } else {
        setCurrentIndex(0);
    }
}

int IntroMenu::nextEnabled(int from, int direction) const
{
    // from + direction부터 한 칸씩 가며 isEnabled()인 첫 줄의 번호를 돌려준다.
    //        목록 끝을 넘어가면 멈추고 from을 돌려준다(순환 없음)
    for (int index = from + direction; index >= 0 && index < int(m_items.size());
         index += direction) {
        if (m_items[index]->isEnabled())
            return index;
    }
    return from;
}

void IntroMenu::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        // 잠긴 줄을 건너뛴다: setCurrentIndex(nextEnabled(m_current, -1))
        setCurrentIndex(nextEnabled(m_current, -1)); // 처음에서 멈춤(순환 없음)
        return;
    case Qt::Key_Down:
        setCurrentIndex(nextEnabled(m_current, +1)); // 끝에서 멈춤
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter: // 숫자 키패드의 Enter
        emit activated(m_current);
        return;
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3: {
        // 수식키(Ctrl · Alt …)가 눌렸으면 "1–3 바로 가기"가 아니다(Ctrl+1 같은 조합은 다른 뜻).
        // 숫자 키패드는 허용.
        if ((event->modifiers() & ~Qt::KeypadModifier) != Qt::NoModifier) {
            QWidget::keyPressEvent(event);
            return;
        }
        const int index = event->key() - Qt::Key_1;
        // 잠긴 줄의 번호 키는 무시한다
        if (!m_items[index]->isEnabled())
            return;
        setCurrentIndex(index);
        emit activated(index);
        return;
    }
    default:
        // 처리하지 않은 키는 베이스로 넘긴다 → 부모 위젯으로 전파된다(Tab 이동 등).
        QWidget::keyPressEvent(event);
    }
}

} // namespace com::yamada::studio
