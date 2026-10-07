#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace com::yamada::studio {
class AppState;
class Repository;
class SpriteCache;
class SquadSession;

// 비전셔틀 창(E2) — 7번째 멤버. 파도타기 · 괴력 같은 비전머신을 본편 대신 드는 요원을 고르고,
// 이 게임의 비전머신마다 누가 드는지 본다: 셔틀(체크) · 본편 멤버(기술 칸을 쓰는 중) · 아무도
// 없음. 본편 6자리 분석(히트맵 · 문제)에는 들어가지 않는다.
class ShuttleDialog : public QDialog
{
    Q_OBJECT
public:
    ShuttleDialog(Repository *repository, AppState *state, SquadSession *session,
                  SpriteCache *icons, QWidget *parent = nullptr);

signals:
    void pickRequested(); // "포켓몬 고르기" — SquadPage가 선택 창을 연다

private:
    void rebuild(); // 세션이 바뀔 때마다 몸통을 처음부터 다시 그린다

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SquadSession *m_session = nullptr;
    SpriteCache *m_icons = nullptr;

    QVBoxLayout *m_layout = nullptr;
    QWidget *m_body = nullptr; // rebuild가 통째로 바꾼다
};
} // namespace com::yamada::studio
