#pragma once

#include <QObject>

namespace com::yamada::studio {
// 앱 전체가 함께 보는 상태. 지금은 "정주행 중인 세대" 하나다(로드맵 A8).
//
// 화면들은 세대를 전역에서 몰래 읽지 않는다(architecture §9). 대신 이 객체의 generationChanged
// 시그널을 받아 자기 화면을 다시 그린다. ROS 2로 치면 latched 토픽 하나 — 누가 바꾸든
// (인트로 · 앱 막대의 세대 버튼) 구독자(도감 …)가 새 값을 받는다.
//
// Q_PROPERTY: 속성을 메타 객체(moc)에 등록한다. 지금 C++ 코드에서는 getter/setter를 직접 부르지만,
// 등록해 두면 나중에 QML이 `appState.generation`으로 읽고 바인딩할 수 있다(data 계층을 QtCore로만
// 두는 이유와 같다).
//
// 값은 QSettings("generation")에 저장된다 → 다음 실행에도 같은 세대로 시작한다.
class AppState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int generation READ generation WRITE setGeneration NOTIFY generationChanged)
public:
    // 고를 수 있는 세대. 늘릴 때는 dexstyle.json(버전 약칭 · 색, 도감 이름)을 먼저 채운다.
    static constexpr int kMinGeneration = 1;
    static constexpr int kMaxGeneration = 9;
    static constexpr int kDefaultGeneration = 4; // 디자인 목업의 기준(신오)

    explicit AppState(QObject *parent = nullptr);

    int generation() const { return m_generation; }
    // 범위 밖이면 무시한다. 같은 값이면 시그널을 보내지 않는다(되먹임 고리 방지).
    void setGeneration(int generation);

signals:
    void generationChanged(int generation);

private:
    int m_generation = kDefaultGeneration;
};
} // namespace com::yamada::studio
