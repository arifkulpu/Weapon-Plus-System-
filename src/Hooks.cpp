#include "Hooks.h"
#include "WeaponUpgradeSystem.h"

namespace plugin {
    void Hooks::install() {
        QuitGameHook::install();
    }

    void Hooks::quitGame() {
        logger::info("Game quitting");
        WeaponUpgradeSystem::getInstance().stopGlowLoop();
    }
}  // namespace plugin
