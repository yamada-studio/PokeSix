#pragma once

#include <QList>
#include <QString>
#include <QWidget>

namespace com::yamada::studio {
class SpriteCache;

// 포켓몬 박스 아이콘 격자(아이템 상세의 "배울 수 있는 포켓몬"). 칸 수는 폭에 맞춰 접히고, 높이는
// heightForWidth로 알린다. 이름은 툴팁으로 — 상세 폭이 좁아서 칸마다 이름을 쓰면 몇 마리 못 보인다.
class PokemonIconGrid : public QWidget
{
public:
    struct Entry
    {
        int pokemonId = 0;
        QString name;
        QString note; // 툴팁 뒤에 붙는 말("SS 한정"). 있으면 칸을 흐리게
    };

    explicit PokemonIconGrid(SpriteCache *icons, QWidget *parent = nullptr);

    void setEntries(const QList<Entry> &entries);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override; // 툴팁: 이름(+ 버전 한정)

private:
    int columns(int width) const;
    int entryAt(const QPoint &pos) const;

    SpriteCache *m_icons = nullptr; // 소유하지 않는다
    QList<Entry> m_entries;
};
} // namespace com::yamada::studio
