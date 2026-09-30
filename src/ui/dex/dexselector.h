#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QButtonGroup;
class QHBoxLayout;

namespace com::yamada::studio {
// 도감 선택 버튼 줄: [전국] [신오 D · P] [신오 Pt] [성도 HG · SS] (도감 백과 머리 띠 오른쪽, D2b).
//
// 버튼 목록은 Repository::dexesForGeneration()이 준다 — 세대마다 어떤 도감이 있는지 코드에 적지
// 않는다. 버튼은 체크 가능한 QPushButton이고 모양은 app.qss(QPushButton#dexButton)가 정한다.
// AppTabBar와 같은 방식: QButtonGroup(하나만 켜짐) + idClicked. 버튼 id = pokedexId(전국 = 0).
class DexSelector : public QWidget
{
    Q_OBJECT
public:
    static constexpr int kNational = 0; // 전국 버튼의 id

    explicit DexSelector(QWidget *parent = nullptr);

    // 버튼을 다시 만든다: [전국] + dexes. 선택은 전국으로 돌아간다(세대가 바뀔 때도 부른다 — A8).
    void setDexes(const QList<DexInfo> &dexes);

signals:
    void dexSelected(int pokedexId); // 사용자가 누를 때만. kNational = 전국

private:
    void addButton(const QString &text, const QString &toolTip, int id);

    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
};
} // namespace com::yamada::studio
