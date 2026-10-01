#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QButtonGroup;
class QHBoxLayout;

namespace com::yamada::studio {
// 포켓몬 선택 창의 도감 칩: [전국][DP][Pt][HGSS] (그 세대의 지방 도감).
//
// 도감 목록 화면의 DexSelector보다 단순하다: 지방 이름 없이 버전 약칭만, 칩 하나 안에서 바탕을
// 버전 수만큼 나눠 각 버전 색(dexstyle.json)으로 칠한다. 같은 게임의 도감이 여럿이면(칼로스 셋)
// 약칭 뒤에 dexstyle의 이름을 붙여 구분한다.
class DexFilterBar : public QWidget
{
    Q_OBJECT
public:
    static constexpr int kNational = 0;

    explicit DexFilterBar(QWidget *parent = nullptr);

    // checkedId: 처음 켤 도감(없으면 전국)
    void setDexes(const QList<DexInfo> &dexes, Language language, int checkedId);
    int currentDex() const;

signals:
    void dexSelected(int pokedexId); // 사용자가 누를 때만. kNational = 전국

private:
    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
};
} // namespace com::yamada::studio
