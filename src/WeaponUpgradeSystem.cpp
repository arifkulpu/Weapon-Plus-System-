#include "WeaponUpgradeSystem.h"
#include <format>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include "RE/I/InterfaceStrings.h"
#include "RE/B/BSTCreateFactoryManager.h"
#include "RE/B/BSTDerivedCreator.h"
#include "RE/B/BSEffectShaderData.h"
#include "RE/E/ExtraTextDisplayData.h"
#include "RE/E/ExtraWorn.h"
#include "RE/S/ScriptEventSourceHolder.h"

namespace RE {
    MessageBoxData::~MessageBoxData() {}
}

namespace plugin {

// MSVC 17.10+ regex linker workaround
extern "C" void __std_regex_transform_primary_char() {}

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    RE::Actor* WeaponUpgradeSystem::getCrosshairActor() const {
        auto crosshairRef = RE::CrosshairPickData::GetSingleton();
        if (!crosshairRef) return nullptr;
        auto target = crosshairRef->target.get();
        if (!target) return nullptr;
        return target->As<RE::Actor>();
    }

    bool WeaponUpgradeSystem::isBlacksmith(RE::Actor* actor) const {
        if (!actor) return false;
        auto faction = RE::TESForm::LookupByEditorID<RE::TESFaction>("JobBlacksmithFaction");
        if (!faction) {
            logger::warn("JobBlacksmithFaction not found - checking by FormID 0x00051599");
            faction = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESFaction>(0x00051599, "Skyrim.esm");
        }
        if (!faction) {
            logger::error("Cannot find JobBlacksmithFaction!");
            return false;
        }
        return actor->IsInFaction(faction);
    }

    RE::TESObjectWEAP* WeaponUpgradeSystem::getEquippedWeapon(RE::FormID& outRefId) const {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return nullptr;
        auto equipped = player->GetEquippedObject(false);
        if (!equipped) return nullptr;
        auto weapon = equipped->As<RE::TESObjectWEAP>();
        if (!weapon) return nullptr;
        outRefId = weapon->GetFormID();
        return weapon;
    }

    RE::TESObjectARMO* WeaponUpgradeSystem::getEquippedShield(RE::FormID& outRefId) const {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return nullptr;
        auto equipped = player->GetEquippedObject(true); // Left hand holds the shield
        if (!equipped) return nullptr;
        auto shield = equipped->As<RE::TESObjectARMO>();
        if (!shield || !shield->IsShield()) return nullptr;
        outRefId = shield->GetFormID();
        return shield;
    }


    // Main entry point
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::onKeyPressed() {
        if (menuOpen_) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto ui = RE::UI::GetSingleton();
        if (!ui || ui->GameIsPaused()) return;

        auto actor = getCrosshairActor();
        if (!actor) return;

        if (!isBlacksmith(actor)) {
            logger::info("Target is not a blacksmith.");
            return;
        }

        RE::FormID itemRefId = 0;
        RE::TESForm* item = getEquippedWeapon(itemRefId);
        if (!item) {
            item = getEquippedShield(itemRefId);
        }

        if (!item) {
            RE::DebugNotification("Silahçı: Önce bir silah veya kalkan kuşanmalısın.");
            return;
        }

        int currentLevel = WeaponUpgradeData::getInstance().getLevel(itemRefId);

        if (currentLevel >= kMaxPlusLevel) {
            RE::DebugNotification("Bu eşya zaten maksimum seviyeye ulaştı (+9)!");
            return;
        }

        showUpgradeMenu(item, itemRefId, currentLevel);
    }

    // -----------------------------------------------------------------------
    // Show upgrade menu
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::showUpgradeMenu(RE::TESForm* item, RE::FormID itemRefId, int currentLevel) {
        menuOpen_ = true;

        const char* rawName = item ? item->GetName() : nullptr;
        std::string itemName = (rawName && rawName[0]) ? rawName : "Bilinmeyen Eşya";

        int cost = upgradeCost(currentLevel);
        float chance = successChance(currentLevel) * 100.0f;

        std::string title = std::format(
            "{} +{}\n\nYükseltme Maliyeti: {} altın\nBaşarı Şansı: {:.0f}%\n\nSeviyeyi +{} yapmak istiyor musun?",
            itemName, currentLevel, cost, chance, currentLevel + 1
        );

        SKSE::log::info("Opening Upgrade Menu for {}", itemName);

        auto callback = RE::make_smart<UpgradeMenuCallback>(item, itemRefId, currentLevel);

        SKSE::log::info("Getting MessageDataFactoryManager...");
        auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
        auto uiStr = RE::InterfaceStrings::GetSingleton();

        SKSE::log::info("Getting Creator for MessageBoxData...");
        auto factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStr->messageBoxData);
        if (!factory) {
            SKSE::log::error("Failed to get creator for MessageBoxData");
            menuOpen_ = false;
            return;
        }

        SKSE::log::info("Creating MessageBoxData instance...");
        auto msgData = factory->Create();
        if (!msgData) {
            SKSE::log::error("MessageBoxData alloc returned null!");
            menuOpen_ = false;
            return;
        }

        SKSE::log::info("Populating MessageBoxData...");
        msgData->bodyText = title;
        msgData->callback = std::move(callback);

        msgData->buttonText.clear();
        msgData->buttonText.push_back("Yükselt");
        msgData->buttonText.push_back("Vazgeç");

        msgData->unk4C = 4;
        msgData->unk4D = 4;
        msgData->unk4E = 2;

        SKSE::log::info("Queueing message via AddTask...");
        SKSE::GetTaskInterface()->AddTask([msgData]() mutable {
            SKSE::log::info("Executing msgData->QueueMessage() inside task...");
            msgData->QueueMessage();
            SKSE::log::info("msgData->QueueMessage() completed.");
        });
    }

    // -----------------------------------------------------------------------
    // Apply upgrade
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::applyUpgrade(RE::TESForm* item, RE::FormID itemRefId, int newLevel) {
        if (!item) return;

        // Try to cast to weapon (shields don't have attack damage)
        auto* weapon = item->As<RE::TESObjectWEAP>();

        static std::unordered_map<RE::FormID, float> originalDamage;

        if (originalDamage.find(itemRefId) == originalDamage.end()) {
            if (weapon) originalDamage[itemRefId] = static_cast<float>(weapon->attackDamage);
            else        originalDamage[itemRefId] = 0.0f; // shields don't have attackDamage
        }

        float orig = originalDamage[itemRefId];
        const char* rawName = item->GetName();
        std::string itemNameStr = (rawName && rawName[0]) ? rawName : "Eşya";

        // Determine upgrade success
        int currentLevel = newLevel - 1;
        float chance = successChance(currentLevel);

        float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        bool success = roll <= chance;

        if (success) {
            // Apply flat damage bonus (only meaningful for weapons)
            if (weapon) {
                float newDmg = orig + static_cast<float>(newLevel);
                weapon->attackDamage = static_cast<uint16_t>(std::round(newDmg));
                logger::info("Item '{}' upgraded SUCCESS to +{}. Damage: {} -> {}",
                    itemNameStr, newLevel, static_cast<int>(orig), weapon->attackDamage);
            } else {
                logger::info("Item '{}' (shield) upgraded SUCCESS to +{}.", itemNameStr, newLevel);
            }

            setWeaponDisplayName(item, newLevel);
            WeaponUpgradeData::getInstance().setLevel(itemRefId, newLevel);

            std::string msg;
            if (weapon)
                msg = std::format("{} +{} seviyesine yükseltildi! (+{} hasar)", itemNameStr, newLevel, newLevel);
            else
                msg = std::format("{} +{} seviyesine yükseltildi!", itemNameStr, newLevel);
            RE::DebugNotification(msg.c_str());
        } else {
            int failedLevel = currentLevel;
            int nextLevel = std::max(0, failedLevel - 1);

            if (weapon) {
                float newDmg = orig + static_cast<float>(nextLevel);
                weapon->attackDamage = static_cast<uint16_t>(std::round(newDmg));
            }

            logger::info("Item '{}' upgraded FAILURE trying +{}. De-leveled: {} -> {}",
                itemNameStr, newLevel, failedLevel, nextLevel);

            setWeaponDisplayName(item, nextLevel);
            WeaponUpgradeData::getInstance().setLevel(itemRefId, nextLevel);

            std::string msg;
            if (nextLevel < failedLevel)
                msg = std::format("{} yükseltmesi başarısız oldu! Seviye +{} değerine düştü.", itemNameStr, nextLevel);
            else
                msg = std::format("{} yükseltmesi başarısız oldu!", itemNameStr);
            RE::DebugNotification(msg.c_str());
        }

        refreshGlow();
    }

    // -----------------------------------------------------------------------
    // Set inventory display name
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::setWeaponDisplayName(RE::TESForm* item, int level) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !item) return;

        const char* rawName = item->GetName();
        std::string baseName = (rawName && rawName[0]) ? rawName : "Eşya";
        std::string newName  = (level > 0) ? std::format("+{} {}", level, baseName) : baseName;

        auto* changes = player->GetInventoryChanges();
        if (!changes || !changes->entryList) return;

        for (auto& entry : *changes->entryList) {
            if (!entry || entry->object != item) continue;
            if (!entry->extraLists) continue;

            for (auto& xList : *entry->extraLists) {
                if (!xList) continue;

                // Update (or create) ExtraTextDisplayData on every instance
                auto* xText = xList->GetByType<RE::ExtraTextDisplayData>();
                if (!xText) {
                    xText = new RE::ExtraTextDisplayData(newName.c_str());
                    xList->Add(xText);
                } else {
                    xText->SetName(newName.c_str());
                }
                SKSE::log::info("Set display name '{}' on weapon instance.", newName);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Glow colour table (R, G, B, baseMult)
    // -----------------------------------------------------------------------

    static std::tuple<float, float, float, float> glowColorForLevel(int level) {
        switch (level) {
            case 1:  return {1.0f, 1.0f,  1.0f,  1.2f};
            case 2:  return {0.6f, 0.85f, 1.0f,  2.0f};
            case 3:  return {0.1f, 0.4f,  1.0f,  2.8f};
            case 4:  return {0.0f, 0.9f,  0.9f,  3.5f};
            case 5:  return {0.1f, 1.0f,  0.3f,  4.0f};
            case 6:  return {1.0f, 1.0f,  0.0f,  4.8f};
            case 7:  return {1.0f, 0.5f,  0.0f,  5.5f};
            case 8:  return {1.0f, 0.15f, 0.0f,  6.5f};
            case 9:  return {1.0f, 0.0f,  0.0f,  8.0f};
            default: return {1.0f, 1.0f,  1.0f,  1.0f};
        }
    }

    static float pulseFrequency(int level) {
        // +1=0.3Hz, +2=0.4Hz, +3=0.5Hz ... +9=1.1Hz
        return 0.3f + (level - 1) * 0.1f;
    }


    static std::pair<float, float> pulseRange(int level) {
        float peakMult = std::get<3>(glowColorForLevel(level));
        float minFactor = 0.05f + (level - 1) * 0.025f;
        float minMult = peakMult * minFactor;
        return { minMult, peakMult };
    }

    void WeaponUpgradeSystem::applyEffectGlow(RE::NiAVObject* node, int level, float t, int depth) {
        if (!node || depth > 20) return;

        auto [r, g, b, baseMult] = glowColorForLevel(level);
        (void)baseMult;

        float freq = pulseFrequency(level);
        auto [minM, maxM] = pulseRange(level);
        float sineVal = 0.5f + 0.5f * std::sin(2.0f * 3.14159265f * freq * t);
        float emissiveMult = minM + (maxM - minM) * sineVal;

        float shiftAmt = std::sin(t * (1.0f + level * 0.15f)) * (0.04f + level * 0.006f);
        float fr = std::clamp(r + shiftAmt, 0.0f, 1.0f);
        float fg = std::clamp(g + shiftAmt * 0.5f, 0.0f, 1.0f);
        float fb = std::clamp(b - shiftAmt * 0.3f, 0.0f, 1.0f);

        if (auto geom = node->AsGeometry()) {
            auto& props = geom->GetGeometryRuntimeData().properties;
            for (auto& p : props) {
                if (!p) continue;
                if (p->GetRTTI() && std::string(p->GetRTTI()->name).find("BSLightingShaderProperty") != std::string::npos) {
                    auto* lp = static_cast<RE::BSLightingShaderProperty*>(p.get());
                    if (lp) {
                        if (lp->emissiveColor) {
                            lp->emissiveColor->red = fr;
                            lp->emissiveColor->green = fg;
                            lp->emissiveColor->blue = fb;
                        }
                        lp->emissiveMult = emissiveMult;
                        lp->flags.set(RE::BSShaderProperty::EShaderPropertyFlag::kOwnEmit);
                    }
                }
            }
        }

        if (auto nodeChild = node->AsNode()) {
            for (auto& child : nodeChild->GetChildren()) {
                if (child) applyEffectGlow(child.get(), level, t, depth + 1);
            }
        }
    }

    void WeaponUpgradeSystem::clearEffectGlow(RE::NiAVObject* node, int depth) {
        if (!node || depth > 50) return;

        if (auto geom = node->AsGeometry()) {
            auto& props = geom->GetGeometryRuntimeData().properties;
            for (auto& p : props) {
                if (!p) continue;
                if (p->GetRTTI() && std::string(p->GetRTTI()->name).find("BSLightingShaderProperty") != std::string::npos) {
                    auto* lp = static_cast<RE::BSLightingShaderProperty*>(p.get());
                    if (lp) {
                        if (lp->emissiveColor) {
                            lp->emissiveColor->red = 0.0f;
                            lp->emissiveColor->green = 0.0f;
                            lp->emissiveColor->blue = 0.0f;
                        }
                        lp->emissiveMult = 1.0f;
                        lp->flags.reset(RE::BSShaderProperty::EShaderPropertyFlag::kOwnEmit);
                    }
                }
            }
        }

        if (auto ninode = node->AsNode()) {
            for (auto& child : ninode->GetChildren()) {
                if (child) clearEffectGlow(child.get(), depth + 1);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Follower glow — scan all nearby actors and apply glow to their equipped
    // upgraded items exactly like we do for the player.
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::applyFollowerGlows(float t) {
        auto& data = WeaponUpgradeData::getInstance();

        // Walk every loaded actor reference in the current cell(s)
        auto processHandle = [&](RE::ActorHandle handle) {
            auto actor = handle.get();
            if (!actor || actor->IsPlayerRef()) return;
            if (!actor->Is3DLoaded()) return;

            // Only process followers (teammates) — skip enemies / civilians
            if (!actor->IsPlayerTeammate()) return;

            // Right hand
            auto* equippedR = actor->GetEquippedObject(false);
            if (equippedR) {
                RE::FormID idR = equippedR->GetFormID();
                int lvR = data.getLevel(idR);
                if (lvR > 0) {
                    for (bool firstPerson : { false, true }) {
                        auto* root = actor->Get3D(firstPerson);
                        if (!root) continue;
                        auto* n = root->GetObjectByName("WEAPON");
                        if (n) applyEffectGlow(n, lvR, t);
                        auto* b = root->GetObjectByName("Bow");
                        if (b) applyEffectGlow(b, lvR, t);
                    }
                }
            }

            // Left hand (weapon or shield)
            auto* equippedL = actor->GetEquippedObject(true);
            if (equippedL) {
                RE::FormID idL = 0;
                auto* wL = equippedL->As<RE::TESObjectWEAP>();
                auto* sL = equippedL->As<RE::TESObjectARMO>();
                if (wL) idL = wL->GetFormID();
                else if (sL && sL->IsShield()) idL = sL->GetFormID();

                if (idL > 0) {
                    int lvL = data.getLevel(idL);
                    if (lvL > 0) {
                        for (bool firstPerson : { false, true }) {
                            auto* root = actor->Get3D(firstPerson);
                            if (!root) continue;
                            auto* n = root->GetObjectByName("WEAPONL");
                            if (n) applyEffectGlow(n, lvL, t);
                            auto* s = root->GetObjectByName("SHIELD");
                            if (s) applyEffectGlow(s, lvL, t);
                        }
                    }
                }
            }
        };

        // Iterate all actor handles in the high-process list (loaded actors)
        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) return;
        for (auto& handle : processLists->highActorHandles) {
            processHandle(handle);
        }
    }

    // -----------------------------------------------------------------------
    // Glow animation — background thread drives time, main thread paints
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::triggerDelayedRefresh() {
        if (!glowEnabled_) return;

        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            SKSE::GetTaskInterface()->AddTask([this]() { refreshGlow(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            SKSE::GetTaskInterface()->AddTask([this]() { refreshGlow(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            SKSE::GetTaskInterface()->AddTask([this]() { refreshGlow(); });
        }).detach();
    }

    // Called from background thread via AddTask — runs on the main game thread.
    void WeaponUpgradeSystem::updateGlowAnimation() {
        if (!glowEnabled_) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->Is3DLoaded()) return;

        int lvR = lastLevelR_.load(std::memory_order_relaxed);
        int lvL = lastLevelL_.load(std::memory_order_relaxed);

        float t = glowTime_.load(std::memory_order_relaxed);

        // Player glow
        std::array<RE::NiAVObject*, 2> roots = { player->Get3D(false), player->Get3D(true) };
        for (auto* root : roots) {
            if (!root) continue;
            if (lvR > 0) {
                auto* n = root->GetObjectByName("WEAPON");
                if (n) applyEffectGlow(n, lvR, t);
                auto* b = root->GetObjectByName("Bow");
                if (b) applyEffectGlow(b, lvR, t);
            }
            if (lvL > 0) {
                auto* n = root->GetObjectByName("WEAPONL");
                if (n) applyEffectGlow(n, lvL, t);
                auto* s = root->GetObjectByName("SHIELD");
                if (s) applyEffectGlow(s, lvL, t);
            }
        }

        // Follower glow
        applyFollowerGlows(t);
    }

    void WeaponUpgradeSystem::startGlowLoop() {
        glowEnabled_ = true;
        animRunning_ = true;
        glowTime_.store(0.0f);

        // Register TESEquipEvent
        auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventSource) {
            eventSource->AddEventSink<RE::TESEquipEvent>(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Registered WeaponUpgradeSystem for TESEquipEvent.");
        } else {
            SKSE::log::error("Failed to get ScriptEventSourceHolder!");
        }

        // Register SKSE::CameraEvent
        auto cameraSource = SKSE::GetCameraEventSource();
        if (cameraSource) {
            cameraSource->AddEventSink(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Registered WeaponUpgradeSystem for CameraEvent.");
        } else {
            SKSE::log::error("Failed to get CameraEventSource!");
        }

        // Register MenuOpenCloseEvent
        auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->AddEventSink<RE::MenuOpenCloseEvent>(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Registered WeaponUpgradeSystem for MenuOpenCloseEvent.");
        } else {
            SKSE::log::error("Failed to get UI for MenuOpenCloseEvent!");
        }

        // Background animation thread — advances glowTime_ and dispatches one
        // AddTask per tick (~30 fps). This is NOT recursive: the thread sleeps
        // between ticks and only ever queues a single task per cycle.
        std::thread([this]() {
            SKSE::log::info("Glow animation thread started.");
            auto lastTime = std::chrono::steady_clock::now();

            while (animRunning_.load(std::memory_order_relaxed)) {
                auto   now = std::chrono::steady_clock::now();
                float  dt  = std::chrono::duration<float>(now - lastTime).count();
                lastTime   = now;

                // Clamp dt to avoid huge jumps after hitching
                dt = std::min(dt, 0.1f);

                // Advance time accumulator
                float t = glowTime_.load(std::memory_order_relaxed) + dt;
                glowTime_.store(t, std::memory_order_relaxed);

                // Only animate when there is something to animate
                if (lastLevelR_.load(std::memory_order_relaxed) > 0 ||
                    lastLevelL_.load(std::memory_order_relaxed) > 0) {
                    SKSE::GetTaskInterface()->AddTask([this]() {
                        updateGlowAnimation();
                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
            }
            SKSE::log::info("Glow animation thread stopped.");
        }).detach();

        // Initial glow application (handles weapons already equipped when loading)
        triggerDelayedRefresh();
    }

    void WeaponUpgradeSystem::stopGlowLoop() {
        animRunning_ = false;
        glowEnabled_ = false;

        auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventSource) {
            eventSource->RemoveEventSink<RE::TESEquipEvent>(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Unregistered WeaponUpgradeSystem from TESEquipEvent.");
        }

        auto cameraSource = SKSE::GetCameraEventSource();
        if (cameraSource) {
            cameraSource->RemoveEventSink(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Unregistered WeaponUpgradeSystem from CameraEvent.");
        }

        auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->RemoveEventSink<RE::MenuOpenCloseEvent>(&WeaponUpgradeSystem::getInstance());
            SKSE::log::info("Unregistered WeaponUpgradeSystem from MenuOpenCloseEvent.");
        }
    }

    RE::BSEventNotifyControl WeaponUpgradeSystem::ProcessEvent(
        const RE::TESEquipEvent* a_event,
        RE::BSTEventSource<RE::TESEquipEvent>*) 
    {
        if (!glowEnabled_) return RE::BSEventNotifyControl::kContinue;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (a_event && a_event->actor && a_event->actor.get() == player) {
            triggerDelayedRefresh();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl WeaponUpgradeSystem::ProcessEvent(
        const SKSE::CameraEvent* a_event,
        RE::BSTEventSource<SKSE::CameraEvent>*) 
    {
        if (!glowEnabled_) return RE::BSEventNotifyControl::kContinue;

        if (a_event) {
            triggerDelayedRefresh();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl WeaponUpgradeSystem::ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) 
    {
        if (!glowEnabled_) return RE::BSEventNotifyControl::kContinue;

        if (a_event && !a_event->opening) {
            std::string_view menuName = a_event->menuName;
            if (menuName == "InventoryMenu" || 
                menuName == "ContainerMenu" || 
                menuName == "MagicMenu" || 
                menuName == "FavoritesMenu" || 
                menuName == "Crafting Menu" ||
                menuName == "BarterMenu") 
            {
                triggerDelayedRefresh();
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    void WeaponUpgradeSystem::refreshGlow() {
        if (!glowEnabled_) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->Is3DLoaded()) return;

        RE::FormID weaponRefIdR = 0;
        try {
            auto equippedR = player->GetEquippedObject(false);
            if (equippedR) {
                auto* w = equippedR->As<RE::TESObjectWEAP>();
                if (w) weaponRefIdR = w->GetFormID();
            }
        } catch (...) { logger::error("Exception reading right-hand in refreshGlow"); }

        RE::FormID weaponRefIdL = 0;
        try {
            auto equippedL = player->GetEquippedObject(true);
            if (equippedL) {
                auto* w = equippedL->As<RE::TESObjectWEAP>();
                if (w) {
                    weaponRefIdL = w->GetFormID();
                } else {
                    auto* s = equippedL->As<RE::TESObjectARMO>();
                    if (s && s->IsShield()) weaponRefIdL = s->GetFormID();
                }
            }
        } catch (...) { logger::error("Exception reading left-hand in refreshGlow"); }

        int levelR = weaponRefIdR > 0 ? WeaponUpgradeData::getInstance().getLevel(weaponRefIdR) : 0;
        int levelL = weaponRefIdL > 0 ? WeaponUpgradeData::getInstance().getLevel(weaponRefIdL) : 0;

        SKSE::log::info("refreshGlow() - R=0x{:X} Lv{}, L=0x{:X} Lv{}",
            weaponRefIdR, levelR, weaponRefIdL, levelL);

        // Cache levels so the animation thread knows what to animate
        lastLevelR_.store(levelR, std::memory_order_relaxed);
        lastLevelL_.store(levelL, std::memory_order_relaxed);

        float t = glowTime_.load(std::memory_order_relaxed);

        // Apply/clear initial glow on both views
        std::array<RE::NiAVObject*, 2> roots = { player->Get3D(false), player->Get3D(true) };
        for (auto* root : roots) {
            if (!root) continue;

            auto* nodeR = root->GetObjectByName("WEAPON");
            if (nodeR) {
                if (levelR > 0) applyEffectGlow(nodeR, levelR, t);
                else            clearEffectGlow(nodeR);
            }
            auto* bowNode = root->GetObjectByName("Bow");
            if (bowNode) {
                if (levelR > 0) applyEffectGlow(bowNode, levelR, t);
                else            clearEffectGlow(bowNode);
            }

            auto* nodeL = root->GetObjectByName("WEAPONL");
            if (nodeL) {
                if (levelL > 0) applyEffectGlow(nodeL, levelL, t);
                else            clearEffectGlow(nodeL);
            }
            auto* shieldNode = root->GetObjectByName("SHIELD");
            if (shieldNode) {
                if (levelL > 0) applyEffectGlow(shieldNode, levelL, t);
                else            clearEffectGlow(shieldNode);
            }
        }
    }


    // -----------------------------------------------------------------------
    // Callback
    // -----------------------------------------------------------------------

    void UpgradeMenuCallback::Run(RE::IMessageBoxCallback::Message a_button) {
        WeaponUpgradeSystem::getInstance().menuOpen_ = false;

        int btnIndex = static_cast<int>(a_button);
        SKSE::log::info("UpgradeMenuCallback::Run triggered with button index: {}", btnIndex);

        if (btnIndex == 0 || btnIndex == 4) {
            SKSE::log::info("Player selected Yükselt (Upgrade). Checking gold...");

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            int cost = upgradeCost(currentLevel_);

            auto inv = player->GetInventory([](const RE::TESBoundObject& obj) {
                return obj.IsGold();
            });

            int goldCount = 0;
            for (auto& [form, data] : inv) {
                goldCount += data.first;
            }

            if (goldCount < cost) {
                std::string msg = std::format("Yeterli altın yok! Gerekli: {} altın (Sahip olduğun: {})", cost, goldCount);
                RE::DebugNotification(msg.c_str());
                return;
            }

            // Deduct gold (FormID 0xF = Gold001)
            auto goldForm = RE::TESForm::LookupByID<RE::TESObjectMISC>(0xF);
            if (goldForm) {
                player->RemoveItem(goldForm, cost, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                SKSE::log::info("Deducted {} gold.", cost);
            } else {
                SKSE::log::error("Could not find Gold form (0xF)!");
            }

            int newLevel = currentLevel_ + 1;
            WeaponUpgradeSystem::getInstance().applyUpgrade(item_, weaponRefId_, newLevel);

        } else {
            logger::info("Upgrade cancelled by player.");
        }
    }

}  // namespace plugin
