#pragma once

#include "data/text/localizedtext.h"

#include <QHash>
#include <QWidget>

class QButtonGroup;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace com::yamada::studio {
class AppState;
class GameSelector;
class MapView;
class PanelFrame;
class Repository;
class SpriteCache;

// 타운맵 백과(T1). 세대 · 게임(버전)을 앱 상태로 따라가고, 두 지방을 오가는 게임(GSC · HGSS)은
// 지방 칩([성도] [관동])으로 나눠 본다. 장소를 고르면 오른쪽에 그 버전의 야생 출현 목록.
class TownMapPage : public QWidget
{
    Q_OBJECT
public:
    TownMapPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

private:
    void refresh(); // 세대 · 게임 · 언어가 바뀌었다
    void selectRegion(const QString &region);
    void showLocation(const QString &location); // 빈 문자열 = 선택 해제(안내 문구)

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SpriteCache *m_pokemonIcons = nullptr;

    GameSelector *m_game = nullptr;
    QHBoxLayout *m_regionChips = nullptr; // 지방 칩들(게임이 바뀌면 다시 만든다)
    QButtonGroup *m_regionGroup = nullptr;
    MapView *m_view = nullptr;
    PanelFrame *m_mapPanel = nullptr;
    PanelFrame *m_detail = nullptr;
    QLabel *m_empty = nullptr;       // 장소를 고르기 전 안내
    QWidget *m_detailBody = nullptr; // 출현 목록(고르면 다시 만든다)
    QVBoxLayout *m_detailLayout = nullptr;

    QString m_version;                     // 지금 게임("soulsilver")
    QString m_region;                      // 지금 지방("johto")
    QHash<QString, LocalizedText> m_names; // 지금 지방의 장소 이름
};
} // namespace com::yamada::studio
