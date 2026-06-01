#include "GameEventHandler.h"
#include "Hooks.h"
#include "InputHandler.h"
#include "Serialization.h"
#include "WeaponUpgradeSystem.h"

namespace plugin {
    void GameEventHandler::onLoad() {
        logger::info("onLoad()");
        Hooks::install();

        // Register SKSE cosave serialization
        Serialization::install();
    }

    void GameEventHandler::onPostLoad() {
        logger::info("onPostLoad()");
    }

    void GameEventHandler::onPostPostLoad() {
        logger::info("onPostPostLoad()");
    }

    void GameEventHandler::onInputLoaded() {
        logger::info("onInputLoaded()");
        // Register input handler once input devices are ready
        InputHandler::getInstance().install();
    }

    void GameEventHandler::onDataLoaded() {
        logger::info("onDataLoaded()");
    }

    void GameEventHandler::onNewGame() {
        logger::info("onNewGame()");
    }

    void GameEventHandler::onPreLoadGame() {
        logger::info("onPreLoadGame()");
        WeaponUpgradeSystem::getInstance().stopGlowLoop();
    }

    void GameEventHandler::onPostLoadGame() {
        logger::info("onPostLoadGame()");
        WeaponUpgradeSystem::getInstance().startGlowLoop();
    }

    void GameEventHandler::onSaveGame() {
        logger::info("onSaveGame()");
    }

    void GameEventHandler::onDeleteGame() {
        logger::info("onDeleteGame()");
    }
}  // namespace plugin