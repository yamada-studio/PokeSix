#include "ui/home/intromenu.h"

#include "ui/home/intromenuitem.h"
#include "ui/theme/tokens.h"

#include <QKeyEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace {
constexpr QMargins kMenuMargins = {6, 6, 6, 6}; // 안쪽 이중 테 안의 padding: 6px
constexpr int kItemSpacing = 2;                 // gap: 2px
constexpr int kItemHeight = com::yamada::studio::tok::kSizeIntroMenuRow; // 58
constexpr int kQuitHeight = 42;
// CSS: 마지막 줄과 종료 줄 사이 = gap 2 + 구분선(margin 4 · 선 2 · margin 4) + gap 2 = 14.
// QBoxLayout은 spacer 주위에 spacing(2)을 한 번만 넣으므로 spacer 자체는 14 − 2.
constexpr int kDividerSpacing = 14 - kItemSpacing;
constexpr int kDividerLine = 2;   // border-top: 2px dashed line
constexpr int kDividerInsetX = 8; // margin: 4px 8px 의 좌우 8
constexpr int kDividerInsetY = 4; // 〃 위 4
// 데이터가 있어야 쓸 수 있는 메뉴 줄(도감 백과 · 아이템 백과 · 스쿼드). 규칙을 if 대신 표로 둔다.
constexpr int kNeedsData[] = {0, 1, 2};
} // namespace

namespace com::yamada::studio {
IntroMenu::IntroMenu(QWidget *parent)
    : QWidget(parent)
{
    // 키보드 이벤트를 받으려면 포커스를 받을 수 있어야 한다. 줄(IntroMenuItem)들은 NoFocus.
    QWidget::setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kMenuMargins);
    layout->setSpacing(kItemSpacing);

    struct Entry
    {
        QString name;
        QString description;
    };
    const Entry entries[] = {
            {tr("도감 백과"), tr("종족값 · 타입 상성 · 습득 기술")},
            {tr("아이템 백과"), tr("회복 · 기술머신 · 진화 · 배틀 · 기타")},
            {tr("스쿼드"), tr("여섯 자리 파티 편성과 타입 분석")},
            {tr("설정"), tr("세대 규칙 · 데이터 · 언어 · 테마")},
    };

    for (const Entry &entry : entries) {
        IntroMenuItem *item = new IntroMenuItem(IntroMenuItem::Kind::Entry);
        item->setText(entry.name);
        item->setDescription(entry.description);
        item->setShortcutText(QString::number(m_items.size() + 1)); // "1" … "4"
        item->setFixedHeight(kItemHeight);
        layout->addWidget(item);
        m_items.push_back(item);
    }

    layout->addSpacing(kDividerSpacing); // 점선은 paintEvent가 이 빈자리에 그린다

    IntroMenuItem *quit = new IntroMenuItem(IntroMenuItem::Kind::Quit);
    quit->setText(tr("종료"));
    quit->setShortcutText(QStringLiteral("Esc"));
    quit->setFixedHeight(kQuitHeight);
    layout->addWidget(quit);
    m_items.push_back(quit);

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
    //        목록 끝을 넘어가면(0보다 작거나 kQuitIndex보다 크면) 멈추고 from을 돌려준다(순환 없음)
    for (int index = from + direction; index >= 0 && index <= kQuitIndex; index += direction) {
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
    case Qt::Key_3:
    case Qt::Key_4: {
        // 수식키(Ctrl · Alt …)가 눌렸으면 "1–4 바로 가기"가 아니다(Ctrl+1 같은 조합은 다른 뜻).
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
    case Qt::Key_Escape:
        if (m_current == kQuitIndex)
            emit activated(kQuitIndex); // 두 번째 Esc = 종료
        else
            setCurrentIndex(kQuitIndex); // 첫 번째 Esc = 종료 줄로 커서만 이동
        return;
    default:
        // 처리하지 않은 키는 베이스로 넘긴다 → 부모 위젯으로 전파된다(Tab 이동 등).
        QWidget::keyPressEvent(event);
    }
}

void IntroMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 마지막 메뉴 줄 아래 빈자리에 2px 점선(line 색)을 그린다.
    const IntroMenuItem *lastEntry = m_items[kQuitIndex - 1];
    const qreal y = lastEntry->geometry().bottom() + 1 + kItemSpacing + kDividerInsetY
                    + kDividerLine / 2.0;
    const qreal left = kMenuMargins.left() + kDividerInsetX;
    const qreal right = width() - kMenuMargins.right() - kDividerInsetX;

    QPainter painter(this);
    QPen pen(QColor(tok::kLine), kDividerLine);
    pen.setDashPattern({3, 3}); // 펜 폭 단위: 6px 선 · 6px 빈칸 (브라우저의 2px dashed와 비슷하게)
    pen.setCapStyle(Qt::FlatCap);
    painter.setPen(pen);
    painter.drawLine(QPointF(left, y), QPointF(right, y));
}
} // namespace com::yamada::studio
