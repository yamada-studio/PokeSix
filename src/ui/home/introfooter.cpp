#include "ui/home/introfooter.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>

namespace {
constexpr int kHintGroupSpacing = 14; // 키 안내 묶음 사이 gap: 14px
constexpr int kKeyCapSpacing = 4;     // 키캡과 설명 사이(원본은 공백 한 칸)

// objectName은 사용자에게 보이지 않는 "식별자"다. app.qss가 QLabel#introKeyCap 처럼 이 이름으로
// 모양을 고른다.
QLabel *makeLabel(const QString &text, const char *objectName)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(QString::fromLatin1(objectName));
    return label;
}
} // namespace

namespace com::yamada::studio {
IntroFooter::IntroFooter(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 왼쪽: 앱 버전. 값은 CMake project(VERSION) → POKESIX_VERSION → main.cpp의
    // setApplicationVersion.
    layout->addWidget(makeLabel(tr("v%1").arg(QApplication::applicationVersion()), "introVersion"));
    layout->addStretch();

    // 가운데: 키 안내 세 묶음. CSS의 margin: 0 auto 처럼 양옆 stretch 사이의 가운데에 놓인다
    // (창의 정가운데가 아니라 좌우 라벨 사이의 가운데).
    struct Hint
    {
        QString key;
        QString text;
    };
    const Hint hints[] = {
            {QStringLiteral("↑↓"), tr("이동")},
            {QStringLiteral("Enter"), tr("선택")},
            {QStringLiteral("1–4"), tr("바로 가기")},
    };
    QHBoxLayout *hintRow = new QHBoxLayout;
    hintRow->setSpacing(kHintGroupSpacing);
    for (const Hint &hint : hints) {
        QHBoxLayout *group = new QHBoxLayout;
        group->setSpacing(kKeyCapSpacing);
        group->addWidget(makeLabel(hint.key, "introKeyCap"));
        group->addWidget(makeLabel(hint.text, "introKeyHint"));
        hintRow->addLayout(group); // 안쪽 레이아웃은 바깥 레이아웃이 소유한다
    }
    layout->addLayout(hintRow);
    layout->addStretch();

    // 오른쪽: 데이터 출처(PokéAPI)와 비공식 도구임을 밝힌다.
    layout->addWidget(makeLabel(tr("데이터: PokéAPI · 비공식 팬 도구"), "introSource"));
}
} // namespace com::yamada::studio
