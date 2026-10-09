#include "ui/theme/theme.h"

#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"

#include <QApplication>
#include <QColor>
#include <QDirIterator>
#include <QFile>
#include <QFontDatabase>
#include <QPalette>
#include <QRegularExpression>
#include <QStyleHints>

#include <algorithm>
#include <array>
#include <string_view>

namespace com::yamada::studio::theme {
namespace {
struct NamedColor
{
    std::string_view name; // tokens.json의 이름. QSS에서는 @ 뒤에 쓴다(@red.deep)
    QRgb value;
};

// 라이트 테마 색 표. tokens.h의 줄 끝 주석(// red.deep)에서 뽑아 만들었다.
// 다크 테마(Phase F3)는 이 표를 바꿔 끼우는 방식으로 붙인다.
constexpr auto kColors = std::to_array<NamedColor>({
        {"red", tok::kRed},
        {"red.deep", tok::kRedDeep},
        {"red.tint", tok::kRedTint},
        {"red.text", tok::kRedText},
        {"white", tok::kWhite},
        {"paper", tok::kPaper},
        {"paper.alt", tok::kPaperAlt},
        {"paper.stripe", tok::kPaperStripe},
        {"blue", tok::kBlue},
        {"blue.deep", tok::kBlueDeep},
        {"blue.tint", tok::kBlueTint},
        {"blue.cell", tok::kBlueCell},
        {"blue.focusRing", tok::kBlueFocusRing},
        {"green", tok::kGreen},
        {"green.tint", tok::kGreenTint},
        {"yellow", tok::kYellow},
        {"yellow.tint", tok::kYellowTint},
        {"yellow.soft", tok::kYellowSoft},
        {"yellow.rowSel", tok::kYellowRowSel},
        {"ink", tok::kInk},
        {"text.1", tok::kText1},
        {"text.2", tok::kText2},
        {"text.3", tok::kText3},
        {"text.disabled", tok::kTextDisabled},
        {"line", tok::kLine},
        {"line.soft", tok::kLineSoft},
        {"line.strong", tok::kLineStrong},
        {"cell.border", tok::kCellBorder},
        {"stat.low", tok::kStatLow},
        {"stat.mid", tok::kStatMid},
        {"stat.good", tok::kStatGood},
        {"stat.high", tok::kStatHigh},
        {"cat.physical", tok::kCatPhysical},
        {"cat.special", tok::kCatSpecial},
        {"cat.status", tok::kCatStatus},
        {"heat.x4", tok::kHeatX4},
        {"heat.x2", tok::kHeatX2},
        {"heat.x2.text", tok::kHeatX2Text},
        {"heat.half", tok::kHeatHalf},
        {"heat.half.text", tok::kHeatHalfText},
        {"heat.quarter", tok::kHeatQuarter},
        {"heat.zero", tok::kHeatZero},
        {"capsule.red", tok::kCapsuleRed},
        {"capsule.cream", tok::kCapsuleCream},
        {"capsule.blush", tok::kCapsuleBlush},
        {"capsule.gloss", tok::kCapsuleGloss},
        {"plate.macTop", tok::kPlateMacTop},
        {"plate.macBottom", tok::kPlateMacBottom},
        {"plate.linuxBase", tok::kPlateLinuxBase},
        {"plate.linuxFace", tok::kPlateLinuxFace},
        {"intro.stripe", tok::kIntroStripe},
        {"intro.titleShadow", tok::kIntroTitleShadow},
        {"menu.pressed", tok::kMenuPressed},
});

// qrc 안의 :/fonts/*.ttf를 전부 등록한다. 등록된 글꼴은 이 프로세스 안에서만 쓸 수 있고,
// 사용자의 시스템에 설치되지는 않는다.
void loadBundledFonts()
{
    QDirIterator it(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf")});
    while (it.hasNext()) {
        const QString path = it.next();
        if (QFontDatabase::addApplicationFont(path) < 0)
            qCWarning(lcUi) << "failed to load font" << path;
    }
}
} // namespace

QString substituteTokens(const QString &styleSheet)
{
    // @ 다음에 영문자로 시작해 영숫자로 끝나는 이름(점 허용): @red, @red.deep, @text.1
    // static: 정규식은 컴파일 비용이 있으니 처음 한 번만 만든다.
    static const QRegularExpression token(QStringLiteral(R"(@([A-Za-z][\w.]*[A-Za-z0-9]))"));

    QString result;
    result.reserve(styleSheet.size());
    qsizetype last = 0;
    for (const QRegularExpressionMatch &m : token.globalMatch(styleSheet)) {
        result += styleSheet.mid(last, m.capturedStart() - last);
        const QByteArray name = m.captured(1).toLatin1();
        const auto found = std::find_if(kColors.begin(), kColors.end(), [&](const NamedColor &c) {
            return c.name == std::string_view(name.constData(), name.size());
        });
        if (found != kColors.end()) {
            result += QColor(found->value).name(QColor::HexRgb).toUpper();
        } else {
            qCWarning(lcUi) << "unknown style token" << m.captured(0);
            result += m.captured(0);
        }
        last = m.capturedEnd();
    }
    result += styleSheet.mid(last);
    return result;
}

void apply(QApplication &app)
{
    loadBundledFonts();

    // 종이 테마는 OS 설정과 무관하게 밝다(다크 테마는 Phase F3에서 토큰 교체로). Windows는
    // Qt 6.5부터 시스템이 다크 모드면 앱 팔레트를 어둡게 바꾸는데, QSS가 `background:
    // transparent`로 비워 둔 자리(분석 창의 스크롤 영역 등)에는 그 어두운 Window 색이 비쳐 검은
    // 상자가 됐다(v0.2.0 msi, 다크 모드 PC). 색 구성표를 밝음으로 고정하고 팔레트의 기본 역할도
    // 토큰으로 채운다 — QSS가 정하지 않은 위젯도 종이 위에 선다.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, QColor(tok::kPaper));
    palette.setColor(QPalette::WindowText, QColor(tok::kText1));
    palette.setColor(QPalette::Base, QColor(tok::kWhite));
    palette.setColor(QPalette::AlternateBase, QColor(tok::kPaper));
    palette.setColor(QPalette::Text, QColor(tok::kText1));
    palette.setColor(QPalette::PlaceholderText, QColor(tok::kText3));
    palette.setColor(QPalette::Button, QColor(tok::kWhite));
    palette.setColor(QPalette::ButtonText, QColor(tok::kText1));
    palette.setColor(QPalette::ToolTipBase, QColor(tok::kWhite));
    palette.setColor(QPalette::ToolTipText, QColor(tok::kText1));
    palette.setColor(QPalette::Highlight, QColor(tok::kYellow));
    palette.setColor(QPalette::HighlightedText, QColor(tok::kInk));
    palette.setColor(QPalette::Mid, QColor(tok::kLine));
    palette.setColor(QPalette::Dark, QColor(tok::kLineStrong));
    app.setPalette(palette);

    QFile file(QStringLiteral(":/styles/app.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcUi) << "cannot open" << file.fileName();
        return;
    }
    app.setStyleSheet(substituteTokens(QString::fromUtf8(file.readAll())));
}

QFont font(const char *family, int pixelSize, QFont::Weight weight)
{
    QFont f(QString::fromLatin1(family));
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    f.setHintingPreference(QFont::PreferNoHinting); // 힌팅이 글자 폭을 바꾸지 않게 →
                                                    // 디자인(브라우저) 수치에 가깝게
    return f;
}
} // namespace com::yamada::studio::theme
