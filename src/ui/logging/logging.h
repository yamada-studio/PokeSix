#pragma once

#include <QLoggingCategory>

namespace com::yamada::studio {
Q_DECLARE_LOGGING_CATEGORY(lcUi)

class Logging
{
public:
    static void initialize();
};
} // namespace com::yamada::studio
