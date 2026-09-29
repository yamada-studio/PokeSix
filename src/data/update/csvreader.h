#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class QIODevice;

namespace com::yamada::studio {
// CSV(RFC 4180) 한 개를 레코드 단위로 읽는다. PokéAPI의 data/v2/csv 파일을 읽기 위해 만들었다.
//
//   QFile file(path);
//   file.open(QIODevice::ReadOnly);
//   CsvReader reader(&file);
//   reader.readHeader();                          // 첫 줄 = 열 이름
//   const int name = reader.column(u"name"_s);    // 열 번호가 아니라 이름으로 찾는다
//   QStringList fields;
//   while (reader.readRecord(fields)) { … fields[name] … }
//   if (reader.hasError()) …
//
// [규칙]
//   - 쉼표로 필드를 나눈다. 필드가 "로 시작하면 다음 "까지가 한 필드이고, 그 안의 쉼표 · 줄바꿈은
//     구분자가 아니다. 따옴표 안의 ""는 따옴표 한 개다.
//       719,9,"10,000,000 Volt Thunderbolt"   → ["719", "9", "10,000,000 Volt Thunderbolt"]
//   - 따옴표 없는 빈 칸은 null QString(isNull() == true) = SQL NULL.
//     따옴표로 감싼 빈 값("")은 null이 아닌 빈 문자열이다. 둘은 뜻이 다르다(값 없음 vs 빈 글자).
//   - 줄 끝은 \n과 \r\n 모두 받는다. 마지막 줄에 줄바꿈이 없어도 된다. 완전히 빈 줄은 건너뛴다.
//   - 인코딩은 UTF-8이다.
//
// QObject를 상속하지 않는다: 시그널 · object tree · 메타 정보가 필요 없는 동기적인 도구라서다.
// 읽을 대상은 QIODevice로 받는다. 앱은 QFile을, 테스트는 QBuffer(메모리 속 문자열)를 넘긴다.
class CsvReader
{
public:
    // device는 이미 열려 있어야 하고, CsvReader보다 오래 살아 있어야 한다(소유하지 않는다).
    explicit CsvReader(QIODevice *device);

    // 다음 레코드를 fields에 채운다. 파일 끝이거나 오류면 false.
    bool readRecord(QStringList &fields);

    // 첫 레코드를 열 이름으로 읽어 둔다. 이후 column(이름)으로 번호를 찾는다.
    bool readHeader();
    int column(const QString &name) const; // 없는 열이면 -1
    const QStringList &header() const { return m_header; }

    // 따옴표가 닫히지 않은 채 파일이 끝나면 오류다. readRecord가 false를 돌려준 뒤 확인한다.
    bool hasError() const { return !m_error.isEmpty(); }
    QString errorString() const { return m_error; }

    // 방금 읽은 레코드가 끝난 줄 번호(1부터). 오류 메시지에 쓴다.
    qint64 lineNumber() const { return m_line; }

private:
    bool readLine(QString &line); // 줄 하나(줄 끝 문자 제외). 파일 끝이면 false

    QIODevice *m_device = nullptr;
    QStringList m_header;
    QHash<QString, int> m_columns;
    qint64 m_line = 0;
    QString m_error;
};
} // namespace com::yamada::studio
