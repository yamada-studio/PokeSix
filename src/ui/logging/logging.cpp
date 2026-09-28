#include "ui/logging/logging.h"

namespace com::yamada::studio {
Q_LOGGING_CATEGORY(lcUi, "pokesix.ui", QtInfoMsg)

void Logging::initialize()
{
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} [%{type}] %{message}");
}
} // namespace com::yamada::studio
