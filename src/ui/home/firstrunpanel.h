#pragma once

#include <QWidget>

class QLabel;
class QStackedWidget;

namespace com::yamada::studio {
class SegmentProgress;
class ShadowButton;

// 첫 실행 패널 — 게임 데이터가 없을 때 인트로의 메뉴 창 위에 끼어든다 (디자인 v2 02b SCR-01 "첫
// 실행", 34 ③).
//
//   Ready        ① 받기 전: 안내 + 예상 크기 + [데이터 받기]
//   Downloading  ② 받는 중: 단계 이름 · 퍼센트 + 10칸 진행 막대 + [취소]
//   Failed       ③ 실패:   빨강 테 + 오류 내용 + [다시 시도]
//
// 이 패널은 데이터를 직접 받지 않는다. 버튼이 눌리면 startRequested() / cancelRequested()를 보내고,
// 진행과 결과는 밖(HomePage가 DataUpdater와 이어 준다)에서 setProgress() / showFailure()로 알려
// 준다. 화면(UI)과 일(data)을 나누는 경계다 — ROS 2로 치면 버튼은 서비스 요청만 보내고, 진행은
// 토픽으로 받는다.
class FirstRunPanel : public QWidget
{
    Q_OBJECT
public:
    enum class State { Ready, Downloading, Failed };

    explicit FirstRunPanel(QWidget *parent = nullptr);

    State state() const { return m_state; }
    void setState(State state);

    void setProgress(int percent, const QString &label); // Downloading 상태로 바꾸고 막대를 채운다
    void showFailure(const QString &detail); // Failed 상태로 바꾸고 오류 내용을 보여 준다

    // 이 상태에서 키보드 포커스를 받을 버튼(첫 포커스 = "데이터 받기", 디자인 02b)
    QWidget *focusTarget() const;

signals:
    void startRequested();  // [데이터 받기] 또는 [다시 시도]
    void cancelRequested(); // [취소]

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QWidget *buildReadyPage();
    QWidget *buildDownloadingPage();
    QWidget *buildFailedPage();

    State m_state = State::Ready;
    QStackedWidget *m_pages = nullptr;
    ShadowButton *m_startButton = nullptr;
    ShadowButton *m_cancelButton = nullptr;
    ShadowButton *m_retryButton = nullptr;
    SegmentProgress *m_progress = nullptr;
    QLabel *m_stepLabel = nullptr;
    QLabel *m_percentLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
};
} // namespace com::yamada::studio
