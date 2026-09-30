#pragma once

#include <QString>

namespace com::yamada::studio {
// 게임 데이터의 표기 언어. 앱이 고르는 언어(AppState::language)와 DB의 이름 열(name_ko/en/ja)이
// 같다. 화면 문구(tr())의 언어와는 별개의 경로다(architecture.md §4) — 둘 다 같은 설정을 따를
// 뿐이다.
enum class Language { Korean, English, Japanese };

// "ko" / "en" / "ja" ↔ Language. 설정 파일 · 명령줄 옵션에 쓴다. 모르는 값이면 fallback.
inline QString languageCode(Language language)
{
    switch (language) {
    case Language::English:
        return QStringLiteral("en");
    case Language::Japanese:
        return QStringLiteral("ja");
    case Language::Korean:
        break;
    }
    return QStringLiteral("ko");
}

inline Language languageFromCode(const QString &code, Language fallback = Language::Korean)
{
    if (code == QLatin1String("ko"))
        return Language::Korean;
    if (code == QLatin1String("en"))
        return Language::English;
    if (code == QLatin1String("ja"))
        return Language::Japanese;
    return fallback;
}

// 세 언어로 된 글자 하나(포켓몬 · 아이템 · 기술 · 지방 이름, 효과 문구).
//
// text(language): 그 언어가 비어 있으면 대신 보일 언어를 정해진 순서로 찾는다.
//   한국어 → 영어 → 일본어   (한국어판이 없던 3세대 아이템은 영어로)
//   영어   → 한국어 → 일본어
//   일본어 → 영어 → 한국어   (PokéAPI의 일본어는 거의 빠짐없다)
// 대신 보이는 글자는 그 언어 사용자가 읽을 가능성이 큰 쪽부터다.
struct LocalizedText
{
    QString ko, en, ja;

    const QString &exact(Language language) const
    {
        return language == Language::English ? en : language == Language::Japanese ? ja : ko;
    }

    QString text(Language language) const
    {
        const QString *order[3] = {&ko, &en, &ja};
        if (language == Language::English)
            order[0] = &en, order[1] = &ko, order[2] = &ja;
        else if (language == Language::Japanese)
            order[0] = &ja, order[1] = &en, order[2] = &ko;
        for (const QString *candidate : order)
            if (!candidate->isEmpty())
                return *candidate;
        return {};
    }

    bool isEmpty() const { return ko.isEmpty() && en.isEmpty() && ja.isEmpty(); }
    // 검색용: 세 언어를 모두 이어 붙인다(어느 언어로 쳐도 찾게)
    QString all() const { return ko + QLatin1Char(' ') + en + QLatin1Char(' ') + ja; }
    void clear() { ko.clear(), en.clear(), ja.clear(); }
};
} // namespace com::yamada::studio
