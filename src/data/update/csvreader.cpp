#include "data/update/csvreader.h"

#include <QIODevice>

namespace com::yamada::studio {
CsvReader::CsvReader(QIODevice *device)
    : m_device(device)
{
}

bool CsvReader::readLine(QString &line)
{
    if (m_device->atEnd())
        return false;

    QByteArray bytes = m_device->readLine(); // 줄 끝 '\n'까지(있으면) 포함해서 돌려준다
    ++m_line;
    if (bytes.endsWith('\n'))
        bytes.chop(1);
    if (bytes.endsWith('\r')) // Windows 줄 끝(\r\n)
        bytes.chop(1);
    line = QString::fromUtf8(bytes);

    // 파일 맨 앞의 BOM(U+FEFF)은 글자가 아니라 "UTF-8입니다"라는 표시다. 첫 열 이름에 붙지 않게
    // 뗀다.
    if (m_line == 1 && line.startsWith(QChar(0xFEFF)))
        line.remove(0, 1);
    return true;
}

bool CsvReader::readRecord(QStringList &fields)
{
    fields.clear();

    // 완전히 빈 줄은 레코드가 아니다(파일 끝의 여분 줄바꿈 등).
    QString line;
    do {
        if (!readLine(line))
            return false;
    } while (line.isEmpty());

    // 상태 기계. 상태는 "지금 따옴표 안인가" 하나뿐이다.
    //   밖: ','  → 필드 끝        맨 앞 '"' → 따옴표 안으로        그 밖 → 글자
    //   안: '""' → 따옴표 한 개    '"'       → 따옴표 밖으로        그 밖(',' 포함) → 글자
    QString field; // null로 시작한다 = "아직 아무것도 없음". 빈 칸이면 null인 채로 나간다(SQL NULL)
    bool quoted = false;
    qsizetype i = 0;

    while (true) {
        if (i == line.size()) { // 줄 끝
            if (quoted) {
                // 따옴표가 아직 열려 있다 = 필드 안에 줄바꿈이 있다. 다음 줄을 이어 읽는다.
                if (!readLine(line)) {
                    m_error = QStringLiteral("line %1: unterminated quoted field").arg(m_line);
                    return false;
                }
                field += QLatin1Char('\n');
                i = 0;
                continue;
            }
            fields.append(field); // 마지막 필드
            return true;
        }

        const QChar c = line.at(i);
        if (quoted) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"'); // "" → "
                    i += 2;
                } else {
                    quoted = false; // 닫는 따옴표
                    ++i;
                }
                continue;
            }
            field += c;
            ++i;
            continue;
        }

        if (c == QLatin1Char(',')) {
            fields.append(field);
            field = QString(); // 다음 필드는 다시 null에서 시작
            ++i;
            continue;
        }
        if (c == QLatin1Char('"') && field.isNull()) {
            quoted = true;
            // 따옴표로 시작한 필드는 비어 있더라도("") "값이 있다". null이 아닌 빈 문자열로 만들어
            // 둔다. null QString에 글자를 하나도 붙이지 않으면 계속 null이기 때문이다.
            field = QStringLiteral("");
            ++i;
            continue;
        }
        field += c;
        ++i;
    }
}

bool CsvReader::readHeader()
{
    if (!readRecord(m_header))
        return false;
    m_columns.clear();
    for (int index = 0; index < m_header.size(); ++index)
        m_columns.insert(m_header.at(index), index);
    return true;
}

int CsvReader::column(const QString &name) const
{
    return m_columns.value(name, -1);
}
} // namespace com::yamada::studio
