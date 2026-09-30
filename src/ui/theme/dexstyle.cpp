#include "ui/theme/dexstyle.h"

#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"

#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
using namespace com::yamada::studio;

constexpr char kPath[] = ":/theme/dexstyle.json";

struct Table
{
    QHash<QString, dexstyle::VersionStyle> versions;
    QHash<QString, dexstyle::DexStyle> dexes;
};

// JSON을 한 번 읽어 표로 만든다. 파일이 없거나 깨졌으면 경고만 남기고 빈 표(모두 기본값)로 간다 —
// 버튼 색이 밋밋해질 뿐 앱은 돈다.
Table load()
{
    Table table;
    QFile file(QString::fromLatin1(kPath));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcUi) << "cannot open" << kPath;
        return table;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (!document.isObject()) {
        qCWarning(lcUi) << kPath << "is not valid JSON:" << error.errorString();
        return table;
    }
    const QJsonObject root = document.object();

    const QJsonObject versions = root.value(QStringLiteral("versions")).toObject();
    for (auto it = versions.begin(); it != versions.end(); ++it) {
        const QJsonObject v = it.value().toObject();
        dexstyle::VersionStyle style;
        style.shortName = v.value(QStringLiteral("short")).toString(it.key());
        // QColor::fromString: "#RRGGBB". 잘못된 값이면 invalid → version()에서 기본색으로
        style.background = QColor::fromString(v.value(QStringLiteral("background")).toString());
        style.text = QColor::fromString(v.value(QStringLiteral("text")).toString());
        table.versions.insert(it.key(), style);
    }

    const QJsonObject dexes = root.value(QStringLiteral("pokedexes")).toObject();
    for (auto it = dexes.begin(); it != dexes.end(); ++it) {
        const QJsonObject d = it.value().toObject();
        table.dexes.insert(it.key(), {d.value(QStringLiteral("label")).toString(),
                                      d.value(QStringLiteral("hidden")).toBool(false)});
    }
    return table;
}

const Table &table()
{
    // 함수 안 static: 처음 부를 때 한 번 초기화된다(C++11부터 스레드 안전). qrc는 앱 시작 때
    // 이미 등록되어 있으므로 여기서 읽어도 된다.
    static const Table instance = load();
    return instance;
}
} // namespace

namespace com::yamada::studio::dexstyle {
VersionStyle version(const QString &identifier, const QString &fallbackName)
{
    VersionStyle style = table().versions.value(identifier);
    if (style.shortName.isEmpty())
        style.shortName = fallbackName;
    if (!style.background.isValid())
        style.background = QColor(tok::kWhite);
    if (!style.text.isValid())
        style.text = QColor(tok::kText1);
    return style;
}

DexStyle dex(const QString &identifier)
{
    return table().dexes.value(identifier);
}
} // namespace com::yamada::studio::dexstyle
