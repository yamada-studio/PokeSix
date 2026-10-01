#include "ui/widgets/searchfield.h"

#include "ui/theme/tokens.h"
#include "ui/widgets/svgicon.h"

#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>

namespace {
constexpr int kBorder = 2;
constexpr int kRadius = 6;
constexpr int kPaddingX = 10;
constexpr int kGap = 8;
constexpr int kIconSize = 16;
const char kSearchSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="%1" )"
          R"(stroke-width="2.2"><circle cx="11" cy="11" r="6"/><path d="M20 20l-4.5-4.5"/></svg>)";
} // namespace

namespace com::yamada::studio {
SearchField::SearchField(const QString &placeholder, const QString &shortcutText, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(kBorder + kPaddingX, 0, kBorder + kPaddingX, 0);
    layout->setSpacing(kGap);

    QLabel *icon = new QLabel;
    icon->setPixmap(svgicon::pixmap(
            QString::fromLatin1(kSearchSvg).arg(QColor(com::yamada::studio::tok::kText3).name()),
            kIconSize, devicePixelRatioF()));
    layout->addWidget(icon);

    m_edit = new QLineEdit;
    m_edit->setObjectName(
            QStringLiteral("searchFieldInput")); // app.qss: 테두리 · 배경 없이(바깥 상자가 그린다)
    m_edit->setPlaceholderText(placeholder);
    m_edit->installEventFilter(this);
    // 확정된 글자가 바뀔 때(영문 · 숫자 입력, 지우기, 붙여 넣기, 한글 조합 확정)
    connect(m_edit, &QLineEdit::textChanged, this, [this] {
        if (m_edit->text().isEmpty())
            m_preedit.clear(); // 다 지웠으면 조합도 끝났다
        emit searchTextChanged(searchText());
    });
    layout->addWidget(m_edit, 1);

    if (!shortcutText.isEmpty()) {
        m_shortcut = new QLabel(shortcutText);
        m_shortcut->setObjectName(QStringLiteral("searchFieldShortcut"));
        layout->addWidget(m_shortcut, 0, Qt::AlignVCenter); // 정렬이 없으면 칸 높이(36)로 늘어난다
    }
    QWidget::setFocusProxy(m_edit); // 이 위젯에 setFocus()하면 입력이 포커스를 받는다

    // 가로는 hint(280)에서 줄거나 늘 수 있고(Preferred), 세로는 36 고정(Fixed).
    // 최소 폭은 setMinimumSize가 아니라 minimumSizeHint() override로 말한다 — setMinimumSize를
    // 부르면 그 값이 hint보다 우선해서 override가 쓰이지 않는다.
    QWidget::setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QWidget::setMaximumWidth(kPreferredWidth);
}

QSize SearchField::sizeHint() const
{
    return QSize(kPreferredWidth, kHeight);
}

QSize SearchField::minimumSizeHint() const
{
    return QSize(kMinimumWidth, kHeight);
}

QString SearchField::searchText() const
{
    // 자모만 있는 조합 글자("ㅇ", "이ㅅ"의 ㅅ)는 아직 글자가 아니다 — 넣으면 결과가 잠깐 0개로
    // 깜빡인다. 한글 호환 자모(U+3131–U+318E)만으로 된 조합은 빼고, 음절이 되면("이") 넣는다.
    const bool onlyJamo = std::all_of(m_preedit.cbegin(), m_preedit.cend(), [](QChar c) {
        return c.unicode() >= 0x3131 && c.unicode() <= 0x318E;
    });
    if (m_preedit.isEmpty() || onlyJamo)
        return m_edit->text();
    QString text = m_edit->text();
    text.insert(m_edit->cursorPosition(), m_preedit); // 조합 중 글자는 커서 자리에 있다
    return text;
}

bool SearchField::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
        QWidget::update(); // 테두리 색(먹 ↔ 파랑)을 바꾸려고 다시 그린다
    if (watched == m_edit && event->type() == QEvent::InputMethod) {
        // 입력기 이벤트: 조합 중 글자(preedit)와 확정 글자(commit)를 함께 싣고 온다. 이 필터는
        // 입력이 이벤트를 처리하기 "전"에 불리므로, 확정 글자가 text()에 들어간 "뒤"에 알리도록
        // 이벤트 루프의 다음 차례로 미룬다(queued). 조합이 끝나면 preedit은 빈 문자열로 온다.
        m_preedit = static_cast<QInputMethodEvent *>(event)->preeditString();
        QMetaObject::invokeMethod(
                this, [this] { emit searchTextChanged(searchText()); }, Qt::QueuedConnection);
    }
    return QWidget::eventFilter(watched, event);
}

void SearchField::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal half = kBorder / 2.0;
    painter.setPen(QPen(QColor(m_edit->hasFocus() ? tok::kBlue : tok::kInk), kBorder));
    painter.setBrush(QColor(tok::kWhite));
    painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), kRadius - half,
                            kRadius - half);
}

void SearchField::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_shortcut) {
        // 폭이 220 이상일 때만 배지를 보인다. 숨긴 라벨은 안쪽 레이아웃에서 자리를 내놓고, 그 폭은
        // 입력 칸(stretch 1)이 가져간다. 이 위젯 자신의 크기는 바깥 레이아웃이 hint만 보고 정하므로
        // 숨기기가 다시 resize를 부르지 않는다(루프 없음).
        m_shortcut->setVisible(event->size().width() >= kShortcutMinWidth);
    }
}
} // namespace com::yamada::studio
