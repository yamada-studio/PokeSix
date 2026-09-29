// PokeSix design tokens v2 — generated from design/tokens.json. Edit tokens.json, not this file.
#pragma once
#include <QtGui/QRgb>
#include <array>

namespace ps::tok {

// Light (default)
inline constexpr QRgb kRed = 0xFFD8322B;  // red
inline constexpr QRgb kRedDeep = 0xFFB8241D;  // red.deep
inline constexpr QRgb kRedTint = 0xFFFDE7E4;  // red.tint
inline constexpr QRgb kRedText = 0xFF8E1C16;  // red.text
inline constexpr QRgb kWhite = 0xFFFFFFFF;  // white
inline constexpr QRgb kPaper = 0xFFF3EFE6;  // paper
inline constexpr QRgb kPaperAlt = 0xFFFAF7F0;  // paper.alt
inline constexpr QRgb kPaperStripe = 0xFFEDE7DA;  // paper.stripe
inline constexpr QRgb kBlue = 0xFF2C64B0;  // blue
inline constexpr QRgb kBlueDeep = 0xFF1D4E8F;  // blue.deep
inline constexpr QRgb kBlueTint = 0xFFE3EDF9;  // blue.tint
inline constexpr QRgb kBlueCell = 0xFFD3E3F7;  // blue.cell
inline constexpr QRgb kBlueFocusRing = 0xFFCFE0F5;  // blue.focusRing
inline constexpr QRgb kGreen = 0xFF2A7D38;  // green
inline constexpr QRgb kGreenTint = 0xFFE3F3E4;  // green.tint
inline constexpr QRgb kYellow = 0xFFF2C12E;  // yellow
inline constexpr QRgb kYellowTint = 0xFFFFF4CC;  // yellow.tint
inline constexpr QRgb kYellowSoft = 0xFFFFE08A;  // yellow.soft
inline constexpr QRgb kYellowRowSel = 0xFFFFF9E0;  // yellow.rowSel
inline constexpr QRgb kInk = 0xFF2A2A33;  // ink
inline constexpr QRgb kText1 = 0xFF22222A;  // text.1
inline constexpr QRgb kText2 = 0xFF55535C;  // text.2
inline constexpr QRgb kText3 = 0xFF6B6872;  // text.3
inline constexpr QRgb kTextDisabled = 0xFF9A96A0;  // text.disabled
inline constexpr QRgb kLine = 0xFFDDD6C8;  // line
inline constexpr QRgb kLineSoft = 0xFFEFEAE0;  // line.soft
inline constexpr QRgb kLineStrong = 0xFFBDB4A3;  // line.strong
inline constexpr QRgb kCellBorder = 0xFFE4DDCF;  // cell.border
inline constexpr QRgb kStatLow = 0xFFE86A4F;  // stat.low
inline constexpr QRgb kStatMid = 0xFFF2C12E;  // stat.mid
inline constexpr QRgb kStatGood = 0xFF5DAE45;  // stat.good
inline constexpr QRgb kStatHigh = 0xFF2C64B0;  // stat.high
inline constexpr QRgb kCatPhysical = 0xFFEE8040;  // cat.physical
inline constexpr QRgb kCatSpecial = 0xFF2C64B0;  // cat.special
inline constexpr QRgb kCatStatus = 0xFFBDBBA6;  // cat.status
inline constexpr QRgb kHeatX4 = 0xFFD8322B;  // heat.x4
inline constexpr QRgb kHeatX2 = 0xFFF9CFC9;  // heat.x2
inline constexpr QRgb kHeatX2Text = 0xFF8E1C16;  // heat.x2.text
inline constexpr QRgb kHeatHalf = 0xFFD3E3F7;  // heat.half
inline constexpr QRgb kHeatHalfText = 0xFF1D4E8F;  // heat.half.text
inline constexpr QRgb kHeatQuarter = 0xFF2C64B0;  // heat.quarter
inline constexpr QRgb kHeatZero = 0xFF2A2A33;  // heat.zero
inline constexpr QRgb kCapsuleRed = 0xFFD8322B;  // capsule.red
inline constexpr QRgb kCapsuleCream = 0xFFFFF8EC;  // capsule.cream
inline constexpr QRgb kCapsuleBlush = 0xFFF4A3A0;  // capsule.blush
inline constexpr QRgb kCapsuleGloss = 0xFFFFFFFF;  // capsule.gloss
inline constexpr QRgb kPlateMacTop = 0xFF3E7FD0;  // plate.macTop
inline constexpr QRgb kPlateMacBottom = 0xFF1D4E8F;  // plate.macBottom
inline constexpr QRgb kPlateLinuxBase = 0xFFB01F19;  // plate.linuxBase
inline constexpr QRgb kPlateLinuxFace = 0xFFFFFDF8;  // plate.linuxFace
inline constexpr QRgb kIntroStripe = 0xFFEFEADF;  // intro.stripe
inline constexpr QRgb kIntroTitleShadow = 0xFFF2C12E;  // intro.titleShadow
inline constexpr QRgb kMenuPressed = 0xFFF2C12E;  // menu.pressed

namespace dark {
inline constexpr QRgb kPaper = 0xFF1B1B21;  // paper
inline constexpr QRgb kWhite = 0xFF25252D;  // white
inline constexpr QRgb kPaperAlt = 0xFF2B2B34;  // paper.alt
inline constexpr QRgb kRaised = 0xFF33333D;  // raised
inline constexpr QRgb kRed = 0xFFB8241D;  // red
inline constexpr QRgb kInk = 0xFF0E0E12;  // ink
inline constexpr QRgb kText1 = 0xFFE9E6EF;  // text.1
inline constexpr QRgb kText2 = 0xFFB9B5C2;  // text.2
inline constexpr QRgb kText3 = 0xFF9F9AA8;  // text.3
inline constexpr QRgb kLine = 0xFF3A3A45;  // line
inline constexpr QRgb kLineSoft = 0xFF2F2F38;  // line.soft
} // namespace dark

struct TypeColor { const char* key; const char16_t* ko; const char16_t* abbr; QRgb fill; QRgb text; };
inline constexpr std::array<TypeColor, 18> kTypes {{
    {"normal", u"노말", u"노", 0xFFBDBBA6, 0xFF2A2A33},
    {"fire", u"불꽃", u"불", 0xFFCC4A22, 0xFFFFFFFF},
    {"water", u"물", u"물", 0xFF3470C2, 0xFFFFFFFF},
    {"grass", u"풀", u"풀", 0xFF377D33, 0xFFFFFFFF},
    {"electric", u"전기", u"전", 0xFFF2C12E, 0xFF2A2A33},
    {"ice", u"얼음", u"얼", 0xFF7FCFD4, 0xFF2A2A33},
    {"fighting", u"격투", u"격", 0xFFA8332A, 0xFFFFFFFF},
    {"poison", u"독", u"독", 0xFF8A3F96, 0xFFFFFFFF},
    {"ground", u"땅", u"땅", 0xFFD8B062, 0xFF2A2A33},
    {"flying", u"비행", u"비", 0xFFA3AEEF, 0xFF2A2A33},
    {"psychic", u"에스퍼", u"에", 0xFFC93866, 0xFFFFFFFF},
    {"bug", u"벌레", u"벌", 0xFFA9BA3A, 0xFF2A2A33},
    {"rock", u"바위", u"바", 0xFF806C30, 0xFFFFFFFF},
    {"ghost", u"고스트", u"고", 0xFF5B4A8C, 0xFFFFFFFF},
    {"dragon", u"드래곤", u"드", 0xFF4B42C4, 0xFFFFFFFF},
    {"dark", u"악", u"악", 0xFF4A3F38, 0xFFFFFFFF},
    {"steel", u"강철", u"강", 0xFFB5C0CC, 0xFF2A2A33},
    {"fairy", u"페어리", u"페", 0xFFEFA6CF, 0xFF2A2A33},
}};

inline constexpr int kSizeAppBar = 60;
inline constexpr int kSizeAppBarCompact = 56;
inline constexpr int kSizeTabInactive = 38;
inline constexpr int kSizeTabActive = 45;
inline constexpr int kSizePanelHeader = 38;
inline constexpr int kSizePanelBorder = 2;
inline constexpr int kSizePanelRadius = 8;
inline constexpr int kSizeHardShadowY = 3;
inline constexpr int kSizeButtonHeight = 36;
inline constexpr int kSizeChipMD = 24;
inline constexpr int kSizeChipSM = 20;
inline constexpr int kSizeChipXS = 22;
inline constexpr int kSizeFilterChip = 30;
inline constexpr int kSizeInputHeight = 36;
inline constexpr int kSizeTableRow = 34;
inline constexpr int kSizeTableRowDense = 30;
inline constexpr int kSizeHeatCellW = 34;
inline constexpr int kSizeHeatCellH = 26;
inline constexpr int kSizeHeatLabelW = 92;
inline constexpr int kSizeHeatGap = 2;
inline constexpr int kSizeFocusRing = 3;
inline constexpr int kSizeSelectOutline = 4;
inline constexpr int kSizeIntroMenuWidth = 520;
inline constexpr int kSizeIntroMenuRow = 58;
inline constexpr int kSizeIntroMenuRowCompact = 52;
inline constexpr int kSizeIntroMenuRowMin = 48;
inline constexpr int kSizeIntroMark = 136;
inline constexpr int kSizeIntroWordmark = 80;
inline constexpr int kSizeAppBarMark = 36;
inline constexpr int kSizeAppBarTabs = 4;
inline constexpr int kBpWide = 1360;
inline constexpr int kBpMedium = 1100;
inline constexpr int kBpNarrow = 1024;
inline constexpr int kBpMin = 960;
inline constexpr int kBpSquadSplit = 1280;
inline constexpr int kBpMinHeight = 640;

// Mark (capsule) — viewBox 100, see docs/01b_DESIGN_SHEET_V2.md §6
inline constexpr double kMarkRotateDeg = -35.0;
inline constexpr double kMarkExtent = 37.3;

} // namespace ps::tok
