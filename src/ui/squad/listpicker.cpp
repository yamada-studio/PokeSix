#include "ui/squad/listpicker.h"

#include "ui/theme/tokens.h"
#include "ui/widgets/rowhover.h"
#include "ui/widgets/searchfield.h"
#include "ui/widgets/shadowbutton.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

constexpr QSize kDialogSize {600, 620};

// 줄 바탕(짝수 줄 paper.alt · 고른 줄 yellow.rowSel · 마우스 줄 yellow.tint)만 칠하고 내용은
// painter 함수에 맡긴다.
class PickerDelegate : public QStyledItemDelegate
{
public:
    PickerDelegate(ListPicker::Painter painter, int rowHeight, QSortFilterProxyModel *proxy,
                   RowHover *hover, QObject *parent)
        : QStyledItemDelegate(parent)
        , m_painter(std::move(painter))
        , m_rowHeight(rowHeight)
        , m_proxy(proxy)
        , m_hover(hover)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return {QStyledItemDelegate::sizeHint(option, index).width(), m_rowHeight};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const bool selected = option.state & QStyle::State_Selected;
        const bool hot = m_hover->row() == index.row();
        QColor background(index.row() % 2 == 1 ? tok::kPaperAlt : tok::kWhite);
        if (hot)
            background = QColor(tok::kYellowTint);
        if (selected)
            background = QColor(tok::kYellowRowSel);
        painter->fillRect(option.rect, background);
        if (selected) { // ▶ 표시 대신 왼쪽 노랑 띠
            painter->fillRect(QRect(option.rect.left(), option.rect.top(), 4, option.rect.height()),
                              QColor(tok::kYellow));
        }
        m_painter(*painter, option.rect, m_proxy->mapToSource(index).row(), hot || selected);
        painter->restore();
    }

private:
    ListPicker::Painter m_painter;
    int m_rowHeight;
    QSortFilterProxyModel *m_proxy;
    RowHover *m_hover;
};
// 칸 제목 띠: 목록 viewport와 같은 가로 자리에 painter(row = −1)를 부른다
class HeaderStrip : public QWidget
{
public:
    HeaderStrip(const ListPicker::Painter &painter, QListView *view, int height)
        : m_painter(painter)
        , m_view(view)
    {
        QWidget::setFixedHeight(height);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        // 목록의 테(2) · 스크롤바만큼 viewport가 안쪽에 있다 → 같은 x에서 시작해 같은 폭으로
        const QPoint origin = m_view->viewport()->mapTo(m_view, QPoint(0, 0));
        m_painter(painter, QRect(origin.x(), 0, m_view->viewport()->width(), height()), -1, false);
    }

private:
    ListPicker::Painter m_painter;
    QListView *m_view;
};
} // namespace

namespace com::yamada::studio {
ListPicker::ListPicker(const QString &title, const QStringList &searchTexts, Painter painter,
                       int rowHeight, QWidget *parent)
    : QDialog(parent)
    , m_model(new QStringListModel(searchTexts, this))
    , m_proxy(new QSortFilterProxyModel(this))
    , m_painter(painter)
{
    QDialog::setWindowTitle(title);
    QDialog::resize(kDialogSize);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    m_headingRow = new QHBoxLayout;
    m_heading = new QLabel(title);
    m_heading->setObjectName(QStringLiteral("dexSectionLabel"));
    m_headingRow->addWidget(m_heading);
    m_headingRow->addStretch();
    layout->addLayout(m_headingRow);
    m_search = new SearchField(tr("이름으로 찾기"));
    m_search->lineEdit()->installEventFilter(this);
    layout->addWidget(m_search);

    m_view = new QListView;
    m_view->setObjectName(QStringLiteral("squadPickerList"));
    m_view->setModel(m_proxy);
    m_view->setUniformItemSizes(true); // 줄 높이가 모두 같다 → 긴 목록(포켓몬 1025)도 빠르다
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_hover = new RowHover(m_view);
    m_view->setItemDelegate(
            new PickerDelegate(std::move(painter), rowHeight, m_proxy, m_hover, m_view));
    m_header = new HeaderStrip(m_painter, m_view, 24);
    m_header->hide();
    layout->addWidget(m_header);
    layout->setSpacing(10);
    layout->addWidget(m_view, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    ShadowButton *none = new ShadowButton(ShadowButton::Variant::Secondary);
    none->hide();
    m_noneButton = none;
    buttons->addWidget(none);
    buttons->addStretch();
    ShadowButton *close = new ShadowButton(ShadowButton::Variant::Secondary);
    close->setText(tr("닫기"));
    buttons->addWidget(close);
    layout->addLayout(buttons);

    connect(m_search, &SearchField::searchTextChanged, m_proxy,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(m_view, &QListView::clicked, this, &ListPicker::choose);
    connect(m_view, &QListView::activated, this, &ListPicker::choose);
    connect(none, &ShadowButton::clicked, this, [this] {
        m_chosen = -1;
        QDialog::accept();
    });
    connect(close, &ShadowButton::clicked, this, &QDialog::reject);
    m_search->lineEdit()->setFocus();
}

void ListPicker::setNoneText(const QString &text)
{
    static_cast<ShadowButton *>(m_noneButton)->setText(text);
    m_noneButton->setVisible(!text.isEmpty());
}

void ListPicker::showHeader(int height)
{
    m_header->setFixedHeight(height);
    m_header->show();
}

void ListPicker::setHeadingWidget(QWidget *widget)
{
    m_headingRow->addWidget(widget);
}

void ListPicker::setHeading(const QString &text)
{
    m_heading->setText(text);
}

void ListPicker::setSearchTexts(const QStringList &searchTexts)
{
    // QStringListModel::setStringList는 모델 리셋 → 프록시 · 뷰가 처음부터 다시 읽는다
    m_model->setStringList(searchTexts);
    m_view->scrollToTop();
}

void ListPicker::setCurrentRow(int row)
{
    const QModelIndex index = m_proxy->mapFromSource(m_model->index(row));
    if (!index.isValid())
        return;
    m_view->setCurrentIndex(index);
    m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void ListPicker::choose(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid())
        return;
    m_chosen = m_proxy->mapToSource(proxyIndex).row();
    QDialog::accept();
}

bool ListPicker::eventFilter(QObject *watched, QEvent *event)
{
    // 검색 칸에 글자를 치면서 ↑↓로 줄을 옮기고 Enter로 고른다(포커스를 목록으로 옮기지 않아도)
    if (watched == m_search->lineEdit() && event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent *>(event)->key();
        const int rows = m_proxy->rowCount();
        int row = m_view->currentIndex().isValid() ? m_view->currentIndex().row() : -1;
        if ((key == Qt::Key_Down || key == Qt::Key_Up) && rows > 0) {
            row = key == Qt::Key_Down ? std::min(rows - 1, row + 1) : std::max(0, row - 1);
            m_view->setCurrentIndex(m_proxy->index(row, 0));
            return true;
        }
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            choose(row >= 0 ? m_proxy->index(row, 0) : m_proxy->index(0, 0));
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
} // namespace com::yamada::studio
