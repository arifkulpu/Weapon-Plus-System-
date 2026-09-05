#pragma once
#include "WeaponUpgradeData.h"
#include <algorithm>
#include <atomic>

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

        /**
         * Toggle glow enabled state dynamically (called from SMF menu).
         */
        void setGlowEnabled(bool enabled);

        /**
         * Performs all pre-upgrade validation checks and opens the upgrade menu.
         * Public so HandSelectionMenuCallback can invoke it after hand selection.
         */
        void processUpgradeCheck(RE::TESForm* item, bool isLeftHand);

        // Called by callbacks to clear the open state
        void setMenuOpen(bool open) { menuOpen_.store(open, std::memory_order_relaxed); }

    private:
        WeaponUpgradeSystem() = default;

        RE::Actor* getCrosshairActor() const;
        bool isBlacksmith(RE::Actor* actor) const;
        void showHandSelectionMenu(RE::TESForm* itemR, int levelR, RE::TESForm* itemL, int levelL);
        void showUpgradeMenu(RE::TESForm* item, RE::FormID weaponRefId, int currentLevel, bool isLeftHand = false);
        void applyUpgrade(RE::TESForm* item, RE::FormID weaponRefId, int newLevel, bool isLeftHand = false);

        // Apply BSEffectShaderData glow to all geometry under a node
        void applyEffectGlow(RE::NiAVObject* node, int level, float t, int depth = 0);
        void clearEffectGlow(RE::NiAVObject* node, int depth = 0);
        void applyFollowerGlows(float t);

        std::atomic<bool> menuOpen_{ false };
        std::atomic<bool> glowEnabled_{ false };

        // Cached upgrade levels — written from main thread on equip, read from anim thread
        std::atomic<int> lastLevelR_{ 0 };
        std::atomic<int> lastLevelL_{ 0 };

        std::atomic<RE::FormID> lastItemR_{ 0 };
        std::atomic<RE::FormID> lastItemL_{ 0 };

        // Animation thread control & lifecycle
        std::atomic<bool>     animRunning_{ false };
        std::atomic<uint64_t> generation_{ 0 };
        std::thread           animThread_;

        // Debounce tracking for delayed refresh
        std::atomic<uint64_t> refreshGen_{ 0 };

        // Time accumulator (seconds) — written by background thread, read by main thread via AddTask
        std::atomic<float> glowTime_{ 0.0f };

        // Throttle counter: follower glows are expensive, only refresh every N ticks
        std::atomic<int> followerGlowThrottle_{ 0 };
    };

    // ---- MessageBox Callbacks ----
    class HandSelectionMenuCallback : public RE::IMessageBoxCallback {
    public:
        HandSelectionMenuCallback(RE::TESForm* itemR, RE::TESForm* itemL)
            : itemR_(itemR), itemL_(itemL) {}

        void Run(RE::IMessageBoxCallback::Message a_button) override;

    private:
        RE::TESForm* itemR_;
        RE::TESForm* itemL_;
    };

    class UpgradeMenuCallback : public RE::IMessageBoxCallback {
    public:
        UpgradeMenuCallback(RE::TESForm* item, RE::FormID weaponRefId, int currentLevel, bool isLeftHand = false)
            : item_(item), weaponRefId_(weaponRefId), currentLevel_(currentLevel), isLeftHand_(isLeftHand) {}

        void Run(RE::IMessageBoxCallback::Message a_button) override;

    private:
        RE::TESForm* item_;
        RE::FormID   weaponRefId_;
        int          currentLevel_;
        bool         isLeftHand_;
    };

}  // namespace plugin
