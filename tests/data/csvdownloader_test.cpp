#include "data/update/csvdownloader.h"
#include "data/update/csvsource.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <set>

using com::yamada::studio::CsvDownloader;
namespace csvsource = com::yamada::studio::csvsource;

namespace {
// "abc"의 SHA-256 (FIPS 180-2 예시 값)
constexpr std::string_view kAbcSha256
        = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

QString writeFile(const QTemporaryDir &dir, const QByteArray &content)
{
    const QString path = dir.filePath(QStringLiteral("sample.csv"));
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(content);
    return path;
}
} // namespace

TEST(CsvDownloader, AcceptsFileWithMatchingSizeAndHash)
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "abc");
    EXPECT_TRUE(CsvDownloader::isValidCopy(path, 3, kAbcSha256));
}

TEST(CsvDownloader, HashComparisonIgnoresCase)
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "abc");
    EXPECT_TRUE(CsvDownloader::isValidCopy(
            path, 3, "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
}

TEST(CsvDownloader, RejectsWrongSizeWrongHashOrMissingFile)
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "abd");
    EXPECT_FALSE(CsvDownloader::isValidCopy(path, 3, kAbcSha256)); // 내용이 다르다
    EXPECT_FALSE(CsvDownloader::isValidCopy(path, 4, kAbcSha256)); // 크기가 다르다
    EXPECT_FALSE(
            CsvDownloader::isValidCopy(dir.filePath(QStringLiteral("none.csv")), 3, kAbcSha256));
}

TEST(CsvDownloader, EmptyDirectoryIsNotComplete)
{
    QTemporaryDir dir;
    EXPECT_FALSE(CsvDownloader(dir.path()).isComplete());
}

TEST(CsvSource, ManifestIsWellFormed)
{
    std::set<std::string_view> names;
    qint64 sum = 0;
    for (const csvsource::CsvFile &file : csvsource::kFiles) {
        EXPECT_TRUE(names.insert(file.name).second) << "duplicate " << file.name;
        EXPECT_EQ(file.sha256.size(), 64u) << file.name;
        EXPECT_GT(file.size, 0) << file.name;
        sum += file.size;
    }
    EXPECT_EQ(sum, csvsource::kTotalSize);
    EXPECT_EQ(std::string_view(csvsource::kCommit).size(), 40u);
}
