// pokesix-read-sav <세이브 파일> [--verbose] [--hex]
//
// 4세대(DP · Pt · HGSS) 세이브의 파티를 core 파서로 읽어 찍는다. 파서의 단계(footer → 슬롯 →
// PKM 풀기 → 필드 → readParty)마다 pokesix.save 카테고리로 로그를 남겨서, 어느 단계에서
// 틀렸는지 바로 보이게 한다. 가이드: docs/guides/h1-save-reader.md — 각 CP의 "② 로그"에서
// 아래 TODO(CPn-log)를 채운다.
#include "core/save/partyreader.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QLoggingCategory>

namespace save = com::yamada::studio::save;

// 로그 카테고리 = 이름 붙은 로거(ROS 2의 logger 이름과 같다). 기본 수준은 Info — Debug 줄은
// --verbose 또는 환경 변수 QT_LOGGING_RULES="pokesix.save.debug=true"로 켠다.
Q_LOGGING_CATEGORY(lcSave, "pokesix.save", QtInfoMsg)

namespace {
// 0x0000CF2C 꼴. width 0 = 자릿수 맞춤 없음
QString hex(std::uint64_t value, int width = 8)
{
    return QStringLiteral("0x")
           + QString::number(value, 16).rightJustified(width, QLatin1Char('0')).toUpper();
}

// 16바이트씩 "0x0008: 88 01 00 00 …" (--hex: 풀린 PKM을 눈으로 볼 때)
void dumpHex(save::Bytes data)
{
    for (std::size_t row = 0; row < data.size(); row += 16) {
        QString line = hex(row, 4) + QStringLiteral(":");
        for (std::size_t i = row; i < std::min(row + 16, data.size()); ++i)
            line += QLatin1Char(' ')
                    + QString::number(data[i], 16).rightJustified(2, QLatin1Char('0'));
        qCInfo(lcSave).noquote() << line;
    }
}

// CP2: 모든 게임 표 × 두 슬롯의 footer — 어느 조합의 CRC가 맞는지가 곧 게임 판별이다
void logFooters(save::Bytes bytes)
{
    for (const save::SaveLayout &layout : save::kGen4Layouts) {
        for (const std::size_t start : {std::size_t {0}, save::kSlotSize}) {
            const save::BlockFooter footer = save::readFooter(bytes, start, layout);
            // TODO(CP2-log) qCDebug(lcSave).noquote() << QStringLiteral("…").arg(…) 로 한 줄:
            //   게임(layout.versionGroup — QString::fromUtf8(layout.versionGroup.data(),
            //   qsizetype(layout.versionGroup.size()))) · 슬롯(start) · major · minor · size ·
            //   magic(hex) · CRC stored/computed(hex(…, 4)) · ok(footer.crcOk())
            qCDebug(lcSave).noquote()
                    << QStringLiteral("%1 slot@%2 major=%3 minor=%4 size=%5 magic=%6 crc=%7/%8 %9")
                               .arg(QString::fromUtf8(layout.versionGroup.data(),
                                                      qsizetype(layout.versionGroup.size())),
                                    -20)
                               .arg(hex(start, 5))
                               .arg(footer.major)
                               .arg(footer.minor)
                               .arg(hex(footer.size, 0))
                               .arg(hex(footer.magic, 8))
                               .arg(hex(footer.storedCrc, 4))
                               .arg(hex(footer.computedCrc, 4))
                               .arg(footer.crcOk() ? "OK" : "--");
        }
    }
}
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // 한 줄: 수준 한 글자 · 카테고리 · 메시지 (앱과 같은 모양, 시각만 뺐다)
    qSetMessagePattern(
            QStringLiteral("%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}"
                           "%{if-critical}C%{endif} %{category}: %{message}"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Read the party from a 4th-gen Pokémon save."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("save"),
                                 QStringLiteral(".sav file (DP, Pt, HGSS)"));
    const QCommandLineOption verbose({QStringLiteral("v"), QStringLiteral("verbose")},
                                     QStringLiteral("Debug log lines (pokesix.save.debug)."));
    const QCommandLineOption hexDump(QStringLiteral("hex"),
                                     QStringLiteral("Dump each decrypted Pokémon (236 bytes)."));
    parser.addOption(verbose);
    parser.addOption(hexDump);
    parser.process(app);
    if (parser.positionalArguments().size() != 1)
        parser.showHelp(1);
    if (parser.isSet(verbose))
        QLoggingCategory::setFilterRules(QStringLiteral("pokesix.save.debug=true"));

    // CP0: 파일 읽기
    const QString path = parser.positionalArguments().constFirst();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(lcSave).noquote() << "cannot open" << path << "—" << file.errorString();
        return 1;
    }
    const QByteArray raw = file.readAll();
    const save::Bytes bytes(reinterpret_cast<const std::uint8_t *>(raw.constData()),
                            std::size_t(raw.size()));
    qCInfo(lcSave).noquote() << QStringLiteral("file %1 — %2 bytes (%3)")
                                        .arg(path)
                                        .arg(raw.size())
                                        .arg(hex(std::uint64_t(raw.size()), 0));
    if (bytes.size() < save::kSaveSize) {
        qCCritical(lcSave) << "smaller than a 4th-gen save (512 KiB) — not a save file";
        return 2;
    }

    // CP2: footer → 게임 · 슬롯
    logFooters(bytes);
    const save::SaveLayout *layout = nullptr;
    std::size_t general = 0;
    for (const save::SaveLayout &candidate : save::kGen4Layouts) {
        if (const auto start = save::activeGeneralBlock(bytes, candidate)) {
            layout = &candidate;
            general = *start;
            break;
        }
    }
    if (!layout) {
        qCCritical(lcSave) << "no valid general block for any 4th-gen game"
                           << "— run with --verbose and read the footer lines";
        return 2;
    }
    const int count = bytes[general + layout->partyCountOffset];
    qCInfo(lcSave).noquote() << QStringLiteral("game %1 · slot at %2 · party %3")
                                        .arg(QString::fromUtf8(
                                                layout->versionGroup.data(),
                                                qsizetype(layout->versionGroup.size())))
                                        .arg(hex(general, 5))
                                        .arg(count);

    // CP3 · CP4: 한 마리씩 — readParty를 거치지 않고 단계 함수를 직접 불러 중간값을 찍는다
    for (int i = 0; i < count && i < 6; ++i) {
        const std::size_t at = general + layout->partyOffset + std::size_t(i) * save::kPartyPkmSize;
        const save::DecodedPkm pkm = save::decodePkm(bytes.subspan(at, save::kPartyPkmSize));
        // TODO(CP3-log) qCDebug: 몇 번째(i + 1) · 파일 위치(hex(at, 5)) · PID(hex) · shuffle 번호 ·
        //   체크섬 stored/computed(hex(…, 4))
        if (!pkm.ok())
            qCWarning(lcSave) << "member" << i + 1 << "checksum mismatch — decryption is wrong";
        if (parser.isSet(hexDump))
            dumpHex(pkm.data);

        const save::ReadMember member = save::parseMember(pkm);
        // TODO(CP4-log) qCInfo().noquote() 한 줄 요약. 예)
        //   "1  #392 Lv.50  HP 100/120  item 0  ability 66  nature 3  moves 7 53 394 0"
        //   알이면 끝에 " (egg)". 폼이 0이 아니면 "#479-1"처럼
        qCInfo(lcSave).noquote() << QStringLiteral("%1  #%2%3 Lv.%4  HP %5/%6  item %7  ability %8 "
                                                   " nature %9  moves %10 %11 %12 %13")
                                                    .arg(i + 1)
                                                    .arg(member.species)
                                                    .arg(member.form != 0
                                                                 ? QStringLiteral("-%1").arg(
                                                                           member.form)
                                                                 : QString())
                                                    .arg(member.level)
                                                    .arg(member.hp)
                                                    .arg(member.maxHp)
                                                    .arg(member.heldItem)
                                                    .arg(member.ability)
                                                    .arg(member.nature)
                                                    .arg(member.moves[0])
                                                    .arg(member.moves[1])
                                                    .arg(member.moves[2])
                                                    .arg(member.moves[3])
                                            + (member.egg ? QStringLiteral(" (egg)") : QString());
    }

    // CP5: 위의 수동 경로와 readParty(앱이 쓸 한 번에 읽기)가 같은 답을 내는지
    const auto party = save::readParty(bytes);

    if (!party.has_value()) {
        qCCritical(lcSave) << "readParty failed — run with --verbose to see why";
        return 3;
    }
    const std::size_t found = party->members.size();
    qCInfo(lcSave).noquote() << QStringLiteral("readParty: %1 · %2 members")
                                        .arg(QString::fromUtf8(
                                                party->layout->versionGroup.data(),
                                                qsizetype(party->layout->versionGroup.size())))
                                        .arg(found);
    if (found != std::size_t(count))
        qCWarning(lcSave) << "readParty found" << found << "members, the step-by-step path"
                          << count;
    for (std::size_t i = 0; i < std::size_t(count) && i < 6; ++i) {
        const std::size_t at = general + layout->partyOffset + i * save::kPartyPkmSize;
        const save::DecodedPkm pkm = save::decodePkm(bytes.subspan(at, save::kPartyPkmSize));
        const save::ReadMember member1 = save::parseMember(pkm);
        if (i >= found) {
            qCWarning(lcSave) << "member" << i + 1 << "missing in readParty";
            continue;
        }
        const save::ReadMember &member2 = party->members[i];
        if (member1.pid != member2.pid)
            qCWarning(lcSave) << "member" << i + 1 << "PID mismatch" << hex(member1.pid)
                              << "!=" << hex(member2.pid);
    }
    return 0;
}
