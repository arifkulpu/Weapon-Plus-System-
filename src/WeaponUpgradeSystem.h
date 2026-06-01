#pragma once
#include "WeaponUpgradeData.h"
#include <algorithm>

namespace plugin {

    /**
     * WeaponUpgradeSystem
     *
     * Handles:
     *  - Detecting if the player's crosshair target is a blacksmith NPC
     *  - Showing the upgrade MessageBox (weapon name, current level, cost)
     *  - Applying damage bonus and glow (BSEffectShaderData) on confirm
     */
    class WeaponUpgradeSystem : public RE::BSTEventSink<RE::TESEquipEvent>,
                                public RE::BSTEventSink<SKSE::CameraEvent>,
                                public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static WeaponUpgradeSystem& getInstance() {
            static WeaponUpgradeSystem instance;
            return instance;
        }

        friend class UpgradeMenuCallback;

        WeaponUpgradeSystem(const WeaponUpgradeSystem&) = delete;
        WeaponUpgradeSystem& operator=(const WeaponUpgradeSystem&) = delete;

        /**
         * Called when the player presses L.
         * Checks conditions and opens the upgrade menu if valid.
         */
        void onKeyPressed();

        /**
         * Event handler for equipment changes (equipping/unequipping items)
         */
        RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event,
                                              RE::BSTEventSource<RE::TESEquipEvent>* a_source) override;

        /**
         * Event handler for camera changes
         */
        RE::BSEventNotifyControl ProcessEvent(const SKSE::CameraEvent* a_event,
                                              RE::BSTEventSource<SKSE::CameraEvent>* a_source) override;

        /**
         * Event handler for menu open/close events
         */
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;

        /**
         * Triggers a multi-stage delayed refresh of the glow effect
         */
        void triggerDelayedRefresh();

        /**
         * Scans equipped weapons and applies/clears glow accordingly.
         */
        void refreshGlow();

        /**
         * Called once per frame to animate the glow pulse effect.
         * Must be called from the main game thread (e.g. Main::Update hook).
         */
        void updateGlowAnimation();

        /**
         * Initialize/register event sinks
         */
        void startGlowLoop();
        void stopGlowLoop();

    private:
        WeaponUpgradeSystem() = default;

        RE::Actor* getCrosshairActor() const;
        bool isBlacksmith(RE::Actor* actor) const;
        RE::TESObjectWEAP* getEquippedWeapon(RE::FormID& outRefId) const;
        void showUpgradeMenu(RE::TESObjectWEAP* weapon, RE::FormID weaponRefId, int currentLevel);
        void applyUpgrade(RE::TESObjectWEAP* weapon, RE::FormID weaponRefId, int newLevel);
        void setWeaponDisplayName(RE::TESObjectWEAP* weapon, int level);

        // Apply BSEffectShaderData glow to all geometry under a node
        void applyEffectGlow(RE::NiAVObject* node, int level, float t, int depth = 0);
        void clearEffectGlow(RE::NiAVObject* node, int depth = 0);
        void printNodeHierarchy(RE::NiAVObject* node, int indent = 0);

        bool menuOpen_ = false;
        bool glowEnabled_ = false;

        // Cached upgrade levels — written from main thread on equip, read from anim thread
        std::atomic<int> lastLevelR_{ 0 };
        std::atomic<int> lastLevelL_{ 0 };

        // Animation thread control
        std::atomic<bool> animRunning_{ false };

        // Time accumulator (seconds) — written by background thread, read by main thread via AddTask
        std::atomic<float> glowTime_{ 0.0f };

        void setGlowEnabled(bool enabled) { glowEnabled_ = enabled; }
    };

    // ---- MessageBox Callback ----
    class UpgradeMenuCallback : public RE::IMessageBoxCallback {
    public:
        UpgradeMenuCallback(RE::TESObjectWEAP* weapon, RE::FormID weaponRefId, int currentLevel)
            : weapon_(weapon), weaponRefId_(weaponRefId), currentLevel_(currentLevel) {}

        void Run(RE::IMessageBoxCallback::Message a_button) override;

    private:
        RE::TESObjectWEAP* weapon_;
        RE::FormID         weaponRefId_;
        int                currentLevel_;
    };

}  // namespace plugin
