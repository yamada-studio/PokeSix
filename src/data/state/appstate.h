#pragma once

#include "data/text/localizedtext.h"

#include <QObject>

namespace com::yamada::studio {
// 앱 전체가 함께 보는 상태: 정주행 중인 세대(로드맵 A8) · 게임(버전)과 표기 언어.
//
// 화면들은 세대를 전역에서 몰래 읽지 않는다(architecture §9). 대신 이 객체의 generationChanged
// 시그널을 받아 자기 화면을 다시 그린다. ROS 2로 치면 latched 토픽 하나 — 누가 바꾸든
// (인트로 · 앱 막대의 세대 버튼) 구독자(도감 …)가 새 값을 받는다.
//
// Q_PROPERTY: 속성을 메타 객체(moc)에 등록한다. 지금 C++ 코드에서는 getter/setter를 직접 부르지만,
// 등록해 두면 나중에 QML이 `appState.generation`으로 읽고 바인딩할 수 있다(data 계층을 QtCore로만
// 두는 이유와 같다).
//
// 값은 QSettings("generation", "games/<세대>", "language")에 저장된다 → 다음 실행에도 같은 세대 ·
// 게임 · 언어로 시작한다.
//
// 게임: 세대마다 정주행 중인 버전 하나("soulsilver"). 같은 묶음(HGSS)이라도 버전마다 따로 깨는
// 롬이라 스쿼드가 따로 있고, 버전 한정 포켓몬 · 아이템이 다르다. 세대를 바꾸면 그 세대에서 마지막에
// 고른 버전으로 돌아간다. 빈 칸 = 아직 안 골랐다(Repository::resolveVersion이 대표 게임을 고른다).
//
// 언어: 게임 데이터 이름(포켓몬 · 아이템 · 기술 · 지방 · 효과 문구)은 languageChanged를 받은 화면이
// 바로 바꿔 그린다. 화면 문구(tr() — 버튼 · 제목)는 앱을 시작할 때 번역 파일을 한 번 불러 정한다
// (main.cpp). 실행 중에 바꾼 언어는 화면 문구에는 다음 실행부터 적용된다.
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

    // 지금 세대의 게임(버전 identifier) · 다른 세대의 게임
    QString game() const { return game(m_generation); }
    QString game(int generation) const;
    // 지금 세대의 게임을 바꾼다. 같은 값이면 시그널 없음
    void setGame(const QString &version);

    Language language() const { return m_language; }
    void setLanguage(Language language); // 같은 값이면 시그널 없음
    // 저장된 언어(앱 시작 때 번역 파일을 고르려고 AppState를 만들기 전에 읽는다) · 저장하기
    static Language savedLanguage();
    static void saveLanguage(Language language);

signals:
    void generationChanged(int generation); // 게임도 그 세대의 것으로 바뀐다(gameChanged는 없다)
    void gameChanged(const QString &version); // 같은 세대 안에서 버전만 바꿨다
    void languageChanged(com::yamada::studio::Language language);

private:
    int m_generation = kDefaultGeneration;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
