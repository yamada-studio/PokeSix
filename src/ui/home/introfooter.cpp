#include "ui/home/introfooter.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>

namespace {
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

    layout->addStretch();

    // 오른쪽: 데이터 출처(PokéAPI)와 비공식 도구임을 밝힌다.
    layout->addWidget(makeLabel(tr("데이터: PokéAPI · 비공식 팬 도구"), "introSource"));
}
} // namespace com::yamada::studio
