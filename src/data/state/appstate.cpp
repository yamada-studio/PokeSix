#include "data/state/appstate.h"

#include "data/logging/logging.h"

#include <QSettings>

namespace {
constexpr char kGenerationKey[] = "generation";
} // namespace

namespace com::yamada::studio {
AppState::AppState(QObject *parent)
    : QObject(parent)
{
    // QSettings(): 조직 · 앱 이름(main.cpp)으로 저장 위치가 정해진다.
    //   Linux ~/.config/YamadaStudio/PokeSix.conf · macOS plist · Windows 레지스트리
    const int saved = QSettings().value(kGenerationKey, kDefaultGeneration).toInt();
    m_generation
            = (saved >= kMinGeneration && saved <= kMaxGeneration) ? saved : kDefaultGeneration;
}

void AppState::setGeneration(int generation)
{
    if (generation < kMinGeneration || generation > kMaxGeneration || generation == m_generation)
        return;
    m_generation = generation;
    QSettings().setValue(kGenerationKey, generation);
    qCInfo(lcData) << "generation" << generation;
    emit generationChanged(generation);
}
} // namespace com::yamada::studio
