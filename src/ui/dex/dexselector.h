#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QAbstractButton;
class QButtonGroup;
class QHBoxLayout;

namespace com::yamada::studio {
// 도감 선택 버튼 줄: [전국] [신오 D|P] [신오 Pt] [성도 HG|SS] (도감 백과 머리 띠 오른쪽, D2b).
//
// 버튼 목록은 Repository::dexesForGeneration()이 준다 — 세대마다 어떤 도감이 있는지 코드에 적지
// 않는다. 버전 배지의 약칭 · 색과 도감 숨김 · 이름은 설정 파일 dexstyle.json(ui/theme/dexstyle.h)이
// 정한다. 버튼은 직접 그린다(배지를 버전마다 다른 색 칸으로 나눠야 해서 QSS로는 안 된다).
// AppTabBar와 같은 방식: QButtonGroup(하나만 켜짐) + idClicked. 버튼 id = pokedexId(전국 = 0).
class DexSelector : public QWidget
{
    Q_OBJECT
public:
    static constexpr int kNational = 0; // 전국 버튼의 id

    explicit DexSelector(QWidget *parent = nullptr);

    // 버튼을 다시 만든다: [전국] + dexes. 선택은 전국으로 돌아간다(세대가 바뀔 때도 부른다 — A8).
    void setDexes(const QList<DexInfo> &dexes);
    // 버튼 글자의 언어(AppState::language). 고른 도감은 그대로 둔다.
    void setLanguage(Language language);
    // 고른 도감의 게임 묶음(상세 화면의 기술 기준). 전국이면 빈 문자열 → 세대의 대표 게임
    QString currentVersionGroup() const;
    int currentDex() const; // 켠 도감의 pokedexId(전국 = kNational)
    // 그 게임의 도감을 켠다(지금 켠 도감이 이미 그 게임 것이거나 전국이면 그대로). 바뀌었으면 true
    // — dexSelected는 보내지 않는다(부른 쪽이 목록을 다시 읽는다)
    bool selectVersionGroup(const QString &versionGroup);
    // [전국] 버튼을 둘지(기본 true). 아이템 백과의 게임 칩처럼 도감이 아니라 게임을 고를 때 false —
    // 그때 버튼 id는 부른 쪽이 정한 DexInfo::pokedexId다. setDexes 전에 부른다
    void setShowNational(bool show) { m_showNational = show; }
    void setCurrent(int id); // 그 버튼을 켠다(dexSelected는 보내지 않는다)

signals:
    void dexSelected(int pokedexId); // 사용자가 누를 때만. kNational = 전국

private:
    void rebuild(int checkedId);
    void addButton(QAbstractButton *button, const QString &toolTip, int id);

    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QList<DexInfo> m_dexes; // 언어를 바꿀 때 버튼을 다시 만들려고 들고 있는다
    Language m_language = Language::Korean;
    bool m_showNational = true;
};
} // namespace com::yamada::studio
