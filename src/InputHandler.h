#pragma once
#include "Config.h"
#include "WeaponUpgradeSystem.h"

namespace plugin {

    /**
     * InputHandler
     *
     * Registers as a BSInputDeviceManager event sink to intercept
     * keyboard presses. On dynamic Configured key, calls WeaponUpgradeSystem.
     */
    class InputHandler : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static InputHandler& getInstance() {
            static InputHandler instance;
            return instance;
        }

        InputHandler(const InputHandler&) = delete;
        InputHandler& operator=(const InputHandler&) = delete;

        void install() {
            auto idm = RE::BSInputDeviceManager::GetSingleton();
            if (!idm) {
                SKSE::log::error("BSInputDeviceManager not found - cannot register input handler!");
                return;
            }
            idm->AddEventSink(this);
            SKSE::log::info("InputHandler registered.");
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                              RE::BSTEventSource<RE::InputEvent*>*) override {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            for (auto* event = *a_event; event; event = event->next) {
                if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) continue;

                auto* btnEvent = static_cast<RE::ButtonEvent*>(event);

                // Only trigger on key-down (not held, not released)
                if (!btnEvent->IsDown()) continue;

                uint32_t targetKey = Config::getInstance().upgradeKey;

                if (btnEvent->GetIDCode() == targetKey &&
                    btnEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {

                    // Dispatch on game thread to safely access game state
                    SKSE::log::info("Upgrade key pressed - triggering weapon upgrade check");
                    SKSE::GetTaskInterface()->AddTask([]() {
                        WeaponUpgradeSystem::getInstance().onKeyPressed();
                    });
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        InputHandler() = default;
    };

}  // namespace plugin
