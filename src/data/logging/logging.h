#pragma once

#include <QLoggingCategory>

namespace com::yamada::studio {
// data 레이어의 로그 카테고리 "pokesix.data". info 이상이 기본으로 보이고, debug는
// QT_LOGGING_RULES="pokesix.data.debug=true" (또는 run.sh --log)로 켠다.
Q_DECLARE_LOGGING_CATEGORY(lcData)
} // namespace com::yamada::studio
