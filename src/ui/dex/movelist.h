#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
class SpriteCache;

// 기술 목록 표 (도감 상세): 레벨업 · 기술머신. 줄이 수십 개뿐이고 고정된 목록이라 모델/뷰 대신
// 위젯 하나가 직접 그린다(스크롤은 상세 화면 전체가 한다).
//   레벨업:   [Lv] [기술] [타입] [분류] [위력] [명중] [PP]
//             하트비늘 아이콘: 기술 떠올리기로만 배우는 기술(MoveEntry::needsReminder — 얻는
//             레벨보다 낮은데 얻을 때 갖고 있지 않은 기술 · 진화 전 단계가 배우지 않는 Lv 1 기술)
//   기술머신: [번호] [기술] [타입] [분류] [위력] [명중] [PP] [획득처] — 획득처는 공략
//   사전(guidebook)
//   가르침 · 알: 첫 칸 없이 [기술]부터 (Mode::Plain)
class MoveList : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { LevelUp, Machine, Plain };

    // icons: 아이템 아이콘 캐시(하트비늘). 소유하지 않는다.
    MoveList(Mode mode, SpriteCache *icons, QWidget *parent = nullptr);

    // versionGroup: 기술머신 획득처 사전의 키("platinum"). generation: 아이콘 모양(1–5세대 BW 그림)
    // userTypes: 이 포켓몬의 타입(저주처럼 쓰는 쪽 타입에 따라 효과가 다른 변화 기술)
    void setMoves(const QList<MoveEntry> &moves, const QString &versionGroup, int generation,
                  const QStringList &userTypes, Language language);

    // 이 게임에서 가장 일찍 얻는 레벨(PokemonDetail::earliestLevel) — 하트비늘 툴팁 문구. 0 = 모름
    void setEarliestLevel(int level) { m_earliestLevel = level; }

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
    // 7 · 8번째 칸은 모드마다 다르다: 레벨업 [효과], 기술머신 [효과][획득처], 가르침 [비용][효과]
    int effectColumn() const { return m_mode == Mode::Plain ? 8 : 7; } // 효과(변화 기술만)
    int costColumn() const { return 7; }                               // 비용(가르침만)
    int placesColumn() const { return 8; }                             // 획득처(기술머신만)

    Mode m_mode;
    SpriteCache *m_icons = nullptr;
    QList<MoveEntry> m_moves;
    QString m_versionGroup;
    QStringList m_userTypes;
    int m_generation = 1;
    int m_earliestLevel = 0;
    QStringList m_costs; // NPC 가르침 비용(줄마다) — Plain 모드
    int m_costWidth = 0; // 비용 칸 폭(가장 긴 비용 글자에 맞춘다)
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
