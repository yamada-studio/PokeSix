#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
class SpriteCache;

// 기술 목록 표 (도감 상세): 레벨업 · 기술머신. 줄이 수십 개뿐이고 고정된 목록이라 모델/뷰 대신
// 위젯 하나가 직접 그린다(스크롤은 상세 화면 전체가 한다).
//   레벨업:   [Lv] [기술] [타입] [분류] [위력] [명중] [PP]
//             Lv 1 기술은 하트비늘 아이콘으로 강조한다 — 레벨업으로 다시 배울 수 없어서, 잊으면
//             기술 떠올리기(하트비늘 1개)로만 되찾는다.
//   기술머신: [번호] [기술] [타입] [분류] [위력] [명중] [PP] [획득처] — 획득처는 공략
//   사전(guidebook)
class MoveList : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { LevelUp, Machine };

    // icons: 아이템 아이콘 캐시(하트비늘). 소유하지 않는다.
    MoveList(Mode mode, SpriteCache *icons, QWidget *parent = nullptr);

    // versionGroup: 기술머신 획득처 사전의 키("platinum"). generation: 아이콘 모양(1–5세대 BW 그림)
    void setMoves(const QList<MoveEntry> &moves, const QString &versionGroup, int generation,
                  Language language);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override; // 획득처 툴팁(잘린 글자 전체)

private:
    struct Column
    {
        int x = 0;
        int width = 0;
    };
    QList<Column> columns() const; // 지금 폭에서 칸 위치
    int rowAt(int y) const;
    QString placesOf(const MoveEntry &move) const;

    Mode m_mode;
    SpriteCache *m_icons = nullptr;
    QList<MoveEntry> m_moves;
    QString m_versionGroup;
    int m_generation = 1;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
