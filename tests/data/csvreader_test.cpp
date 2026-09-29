#include "data/update/csvreader.h"

#include <QBuffer>
#include <QFile>

#include <gtest/gtest.h>

using com::yamada::studio::CsvReader;

namespace {
// 문자열을 "파일처럼" 읽게 해 주는 작은 도우미. QBuffer도 QIODevice라서 CsvReader가 그대로 받는다.
class CsvText
{
public:
    explicit CsvText(const QByteArray &text)
    {
        m_buffer.setData(text);
        m_buffer.open(QIODevice::ReadOnly);
    }
    QIODevice *device() { return &m_buffer; }

private:
    QBuffer m_buffer;
};

QStringList readAll(const QByteArray &text)
{
    CsvText csv(text);
    CsvReader reader(csv.device());
    QStringList fields;
    QStringList flat;
    while (reader.readRecord(fields))
        flat << fields.join(QLatin1Char('|'));
    return flat;
}
} // namespace

TEST(CsvReader, SplitsPlainFields)
{
    CsvText csv("1,bulbasaur,1\n");
    CsvReader reader(csv.device());
    QStringList fields;
    ASSERT_TRUE(reader.readRecord(fields));
    EXPECT_EQ(fields, (QStringList {"1", "bulbasaur", "1"}));
    EXPECT_FALSE(reader.readRecord(fields));
    EXPECT_FALSE(reader.hasError());
}

TEST(CsvReader, EmptyFieldIsNullButQuotedEmptyIsNot)
{
    CsvText csv("1,,\"\"\n"); // pokemon_species.csv의 빈 evolves_from_species_id 같은 경우 + 따옴표
                              // 빈 값
    CsvReader reader(csv.device());
    QStringList fields;
    ASSERT_TRUE(reader.readRecord(fields));
    ASSERT_EQ(fields.size(), 3);
    EXPECT_TRUE(fields[1].isNull());  // 값 없음 → SQL NULL
    EXPECT_FALSE(fields[2].isNull()); // "" → 빈 문자열
    EXPECT_TRUE(fields[2].isEmpty());
}

TEST(CsvReader, TrailingEmptyFieldIsKept)
{
    // pokemon_species.csv의 줄은 마지막 열(conquest_order)이 비어 있어서 쉼표로 끝난다.
    CsvText csv("1,2,\n");
    CsvReader reader(csv.device());
    QStringList fields;
    ASSERT_TRUE(reader.readRecord(fields));
    ASSERT_EQ(fields.size(), 3);
    EXPECT_TRUE(fields[2].isNull());
}

TEST(CsvReader, CommaInsideQuotesIsNotASeparator)
{
    // 실제 데이터: move_names.csv
    EXPECT_EQ(readAll("719,9,\"10,000,000 Volt Thunderbolt\"\n"),
              (QStringList {"719|9|10,000,000 Volt Thunderbolt"}));
}

TEST(CsvReader, DoubledQuoteIsOneQuote)
{
    EXPECT_EQ(readAll("1,\"say \"\"hi\"\"\"\n"), (QStringList {"1|say \"hi\""}));
}

TEST(CsvReader, NewlineInsideQuotesContinuesTheField)
{
    CsvText csv("1,\"line one\nline two\",x\n2,b,c\n");
    CsvReader reader(csv.device());
    QStringList fields;
    ASSERT_TRUE(reader.readRecord(fields));
    EXPECT_EQ(fields, (QStringList {"1", "line one\nline two", "x"}));
    EXPECT_EQ(reader.lineNumber(), 2); // 레코드 하나가 두 줄에 걸쳐 있었다
    ASSERT_TRUE(reader.readRecord(fields));
    EXPECT_EQ(fields, (QStringList {"2", "b", "c"}));
}

TEST(CsvReader, AcceptsCrLfMissingFinalNewlineAndBlankLines)
{
    EXPECT_EQ(readAll("a,b\r\nc,d\r\n\r\ne,f"), (QStringList {"a|b", "c|d", "e|f"}));
}

TEST(CsvReader, DecodesUtf8AndStripsBom)
{
    CsvText csv("\xEF\xBB\xBFid,name\n1,이상해씨\n");
    CsvReader reader(csv.device());
    ASSERT_TRUE(reader.readHeader());
    EXPECT_EQ(reader.column(QStringLiteral("id")), 0); // BOM이 "id" 앞에 붙지 않았다
    QStringList fields;
    ASSERT_TRUE(reader.readRecord(fields));
    EXPECT_EQ(fields[1], QStringLiteral("이상해씨"));
}

TEST(CsvReader, FindsColumnsByName)
{
    CsvText csv("pokemon_species_id,local_language_id,name,genus\n1,3,이상해씨,씨앗포켓몬\n");
    CsvReader reader(csv.device());
    ASSERT_TRUE(reader.readHeader());
    EXPECT_EQ(reader.column(QStringLiteral("name")), 2);
    EXPECT_EQ(reader.column(QStringLiteral("missing")), -1);
}

TEST(CsvReader, UnterminatedQuoteIsAnError)
{
    CsvText csv("1,\"never closed\n2,x\n");
    CsvReader reader(csv.device());
    QStringList fields;
    EXPECT_FALSE(reader.readRecord(fields));
    EXPECT_TRUE(reader.hasError());
}

// 실제 PokéAPI 파일로도 확인한다. pokesix-fetch-csv로 받아 둔 경우에만 돈다(없으면 건너뜀).
TEST(CsvReader, ReadsTheRealMoveNamesFile)
{
    const QByteArray dir = qgetenv("POKESIX_CSV_DIR");
    if (dir.isEmpty())
        GTEST_SKIP() << "set POKESIX_CSV_DIR to a folder downloaded by pokesix-fetch-csv";
    QFile file(QString::fromLocal8Bit(dir) + QStringLiteral("/move_names.csv"));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    CsvReader reader(&file);
    ASSERT_TRUE(reader.readHeader());
    const int name = reader.column(QStringLiteral("name"));
    QStringList fields;
    int rows = 0;
    bool foundVolt = false;
    while (reader.readRecord(fields)) {
        ASSERT_EQ(fields.size(), reader.header().size()) << "line " << reader.lineNumber();
        foundVolt = foundVolt || fields[name] == QStringLiteral("10,000,000 Volt Thunderbolt");
        ++rows;
    }
    EXPECT_FALSE(reader.hasError());
    EXPECT_GT(rows, 5000);
    EXPECT_TRUE(foundVolt);
}
