#pragma once

#include "data/text/localizedtext.h"

#include <QSet>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLabel;

namespace com::yamada::studio {
class RangeSlider;
class ShadowButton;

// 도감 목록의 필터 창 내용 (02 SCR-02 왼쪽 창, E1). PanelFrame(파랑 머리 "필터") 안에 들어간다.
//
//   타입          [노말][불꽃][물] …   ← 그 세대의 타입만, 여러 개 고르면 "하나라도 가진"
//   종족값 합계    ●──────●  500 – 720
//   ☐ 전설 · 환상 제외
//   ☐ 최종 진화만                      ← 그 세대 기준(1세대 롱스톤은 최종 진화)
//   [필터 초기화]
//
// 조건이 바뀔 때마다 changed()만 보낸다. 실제로 거르는 쪽(SpeciesFilterProxy)과 어떻게 이을지는
// DexPage가 정한다.
class DexFilterPanel : public QWidget
{
    Q_OBJECT
public:
    // 슬라이더 범위. 실제 합계는 대략 175–720이라 양 끝 = "제한 없음"으로 본다(noLimit…)
    static constexpr int kTotalMinimum = 150;
    static constexpr int kTotalMaximum = 800;
    static constexpr int kTotalStep = 10;

    explicit DexFilterPanel(QWidget *parent = nullptr);

    // 그 세대의 타입으로 칩을 다시 만든다(고른 것은 풀린다). 세대 · 언어가 바뀔 때 부른다.
    void setTypes(const QStringList &types, Language language);
    void reset(); // 전부 기본값으로 (changed는 바뀐 것이 있을 때만)

    QSet<QString> selectedTypes() const;
    int minimumTotal() const; // 왼쪽 끝이면 0 (제한 없음)
    int maximumTotal() const; // 오른쪽 끝이면 매우 큰 값 (제한 없음)
    bool excludeLegendary() const;
    bool finalEvolutionOnly() const;

signals:
    void changed();

private:
    void updateTotalLabel();

    QGridLayout *m_typeGrid = nullptr;
    QList<class TypeToggle *> m_typeButtons;
    RangeSlider *m_total = nullptr;
    QLabel *m_totalLabel = nullptr;
    QCheckBox *m_legendary = nullptr;
    QCheckBox *m_finalOnly = nullptr;
    ShadowButton *m_reset = nullptr;
};
} // namespace com::yamada::studio
