#include "WeaponUpgradeSystem.h"
#include <format>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <random>
#include "RE/I/InterfaceStrings.h"
#include "RE/B/BSTCreateFactoryManager.h"
#include "RE/B/BSTDerivedCreator.h"
#include "RE/B/BSEffectShaderData.h"
#include "RE/E/ExtraTextDisplayData.h"
#include "RE/B/BipedAnim.h"
#include "RE/B/BipedObjects.h"
#include "RE/A/Actor.h"
#include "RE/B/BSLightingShaderProperty.h"
#include "RE/E/ExtraHealth.h"
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

    // -----------------------------------------------------------------------
    // Per-instance helpers
    // -----------------------------------------------------------------------

    // Returns the ExtraDataList that carries the kWorn/kWornLeft flag for 'item' in
    // the inventory (i.e. the exact stack that is equipped).
    static RE::ExtraDataList* getWornExtraList(RE::Actor* actor, RE::TESForm* item, bool leftHand = false) {
        if (!actor || !item) return nullptr;
        auto* changes = actor->GetInventoryChanges();
        if (!changes || !changes->entryList) return nullptr;
        auto targetExtraType = leftHand ? RE::ExtraDataType::kWornLeft : RE::ExtraDataType::kWorn;
        for (auto* entry : *changes->entryList) {
            if (!entry || entry->object != item || !entry->extraLists) continue;
            for (auto* xList : *entry->extraLists) {
                if (xList && xList->HasType(targetExtraType))
                    return xList;
            }
        }
        return nullptr;
    }

    // Parses the upgrade level encoded in our custom display name.
    // Expected format: "+N WeaponName" e.g. "+3 Iron Sword"
    // Returns 0 if the ExtraDataList has no custom name or the format is absent.
    static int parseLevelFromExtraList(RE::ExtraDataList* xList) {
        if (!xList) return 0;
        auto* xText = xList->GetByType<RE::ExtraTextDisplayData>();
        if (!xText) return 0;
        std::string name = xText->displayName.c_str();
        SKSE::log::trace("parseLevelFromExtraList: Found displayName '{}'", name);
        if (name.empty() || name[0] != '+') return 0;
        
        auto spacePos = name.find(' ');
        if (spacePos == std::string::npos) return 0;
        
        try { 
            int parsed = std::stoi(name.substr(1, spacePos - 1));
            SKSE::log::trace("parseLevelFromExtraList: Parsed level {}", parsed);
            return parsed;
        }
        catch (...) { return 0; }
    }

    // -----------------------------------------------------------------------
    // Main entry point
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::onKeyPressed() {
        if (menuOpen_.load(std::memory_order_relaxed)) return;

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

        RE::TESForm* itemR = nullptr;
        RE::TESForm* itemL = nullptr;

        if (auto equippedR = player->GetEquippedObject(false)) {
            if (equippedR->IsWeapon()) {
                itemR = equippedR;
            }
        }

        if (auto equippedL = player->GetEquippedObject(true)) {
            if (equippedL->IsWeapon()) {
                itemL = equippedL;
            }
        }

        if (!itemR && !itemL) {
            RE::DebugNotification("Blacksmith: You must equip a weapon first.");
            return;
        }

        auto* weapR = itemR ? itemR->As<RE::TESObjectWEAP>() : nullptr;
        bool is2H = false;
        if (weapR) {
            auto type = weapR->GetWeaponType();
            if (type == RE::WEAPON_TYPE::kTwoHandAxe || 
                type == RE::WEAPON_TYPE::kTwoHandSword ||
                type == RE::WEAPON_TYPE::kBow ||
                type == RE::WEAPON_TYPE::kCrossbow) {
                is2H = true;
            }
        }

        if (itemR && itemL && !is2H) {
            // Both hands have one-handed weapons -> open Hand Selection Menu
            RE::ExtraDataList* wornListR = getWornExtraList(player, itemR, false);
            int levelR = parseLevelFromExtraList(wornListR);

            RE::ExtraDataList* wornListL = getWornExtraList(player, itemL, true);
            int levelL = parseLevelFromExtraList(wornListL);

            showHandSelectionMenu(itemR, levelR, itemL, levelL);
        } else if (itemR) {
            // Only Right Hand weapon
            processUpgradeCheck(itemR, false);
        } else if (itemL) {
            // Only Left Hand weapon
            processUpgradeCheck(itemL, true);
        }
    }

    void WeaponUpgradeSystem::processUpgradeCheck(RE::TESForm* item, bool isLeftHand) {
        if (!item) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        RE::ExtraDataList* wornList = getWornExtraList(player, item, isLeftHand);
        int currentLevel = parseLevelFromExtraList(wornList);

        // Smithing skill check
        float smithingSkill = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSmithing);
        int requiredSmithing = (currentLevel + 1) * 10;
        if (smithingSkill < requiredSmithing) {
            std::string msg = std::format("You need {} Smithing to upgrade to +{} (Current: {:.0f}).", requiredSmithing, currentLevel + 1, smithingSkill);
            RE::DebugNotification(msg.c_str());
            return;
        }

        int maxLvl = Config::getInstance().maxUpgradeLevel;
        if (currentLevel >= maxLvl) {
            std::string msg = std::format("This weapon has already reached maximum level (+{})!", maxLvl);
            RE::DebugNotification(msg.c_str());
            return;
        }

        showUpgradeMenu(item, item->GetFormID(), currentLevel, isLeftHand);
    }

    // -----------------------------------------------------------------------
    // Show Hand Selection Menu (Dual Wielding)
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::showHandSelectionMenu(RE::TESForm* itemR, int levelR, RE::TESForm* itemL, int levelL) {
        menuOpen_.store(true, std::memory_order_relaxed);

        std::string nameR = (itemR && itemR->GetName()) ? itemR->GetName() : "Weapon";
        std::string nameL = (itemL && itemL->GetName()) ? itemL->GetName() : "Weapon";

        std::string title = "Blacksmith: Select Weapon to Upgrade\n\nWhich equipped weapon would you like to upgrade?";

        SKSE::log::info("Opening Hand Selection Menu");

        auto callback = RE::make_smart<HandSelectionMenuCallback>(itemR, itemL);

        auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
        auto uiStr = RE::InterfaceStrings::GetSingleton();
        auto factory = factoryManager ? factoryManager->GetCreator<RE::MessageBoxData>(uiStr->messageBoxData) : nullptr;
        if (!factory) {
            menuOpen_.store(false, std::memory_order_relaxed);
            return;
        }

        auto msgData = factory->Create();
        if (!msgData) {
            menuOpen_.store(false, std::memory_order_relaxed);
            return;
        }

        msgData->bodyText = title;
        msgData->callback = std::move(callback);

        msgData->buttonText.clear();
        msgData->buttonText.push_back(std::format("Right Hand: {} (+{})", nameR, levelR).c_str());
        msgData->buttonText.push_back(std::format("Left Hand: {} (+{})", nameL, levelL).c_str());
        msgData->buttonText.push_back("Cancel");

        msgData->unk4C = 4;
        msgData->unk4D = 4;
        msgData->unk4E = 2;

        SKSE::GetTaskInterface()->AddTask([msgData]() mutable {
            msgData->QueueMessage();
        });
    }

    // -----------------------------------------------------------------------
    // Show upgrade menu
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::showUpgradeMenu(RE::TESForm* item, RE::FormID itemRefId, int currentLevel, bool isLeftHand) {
        menuOpen_.store(true, std::memory_order_relaxed);

        const char* rawName = item ? item->GetName() : nullptr;
        std::string itemName = (rawName && rawName[0]) ? rawName : "Unknown Weapon";

        int cost = upgradeCost(currentLevel);
        float chance = successChance(currentLevel) * 100.0f;

        std::string handStr = isLeftHand ? " (Left Hand)" : " (Right Hand)";
        std::string title = std::format(
            "{}{} +{}\n\nUpgrade Cost: {} gold\nSuccess Chance: {:.0f}%\n\nDo you want to upgrade this weapon to +{}?",
            itemName, handStr, currentLevel, cost, chance, currentLevel + 1
        );

        SKSE::log::info("Opening Upgrade Menu for {}", itemName);

        auto callback = RE::make_smart<UpgradeMenuCallback>(item, itemRefId, currentLevel, isLeftHand);

        auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
        auto uiStr = RE::InterfaceStrings::GetSingleton();

        auto factory = factoryManager ? factoryManager->GetCreator<RE::MessageBoxData>(uiStr->messageBoxData) : nullptr;
        if (!factory) {
            menuOpen_.store(false, std::memory_order_relaxed);
            return;
        }

        auto msgData = factory->Create();
        if (!msgData) {
            menuOpen_.store(false, std::memory_order_relaxed);
            return;
        }

        msgData->bodyText = title;
        msgData->callback = std::move(callback);

        msgData->buttonText.clear();
        msgData->buttonText.push_back("Upgrade");
        msgData->buttonText.push_back("Cancel");

        msgData->unk4C = 4;
        msgData->unk4D = 4;
        msgData->unk4E = 2;

        SKSE::GetTaskInterface()->AddTask([msgData]() mutable {
            msgData->QueueMessage();
        });
    }

    // -----------------------------------------------------------------------
    // Set inventory display name
    // -----------------------------------------------------------------------

    static void setExtraTextDisplayData(RE::ExtraDataList* xList, RE::TESForm* item, int level) {
        if (!xList) return;
        auto* xText = xList->GetByType<RE::ExtraTextDisplayData>();
        std::string baseName = item ? item->GetName() : "Item";
        if (baseName.empty()) baseName = "Weapon";

        std::string newName = (level > 0)
            ? std::format("+{} {}", level, baseName)
            : baseName;

        if (!xText) {
            xText = new RE::ExtraTextDisplayData(newName.c_str());
            xText->customNameLength = static_cast<std::int16_t>(newName.size());
            xList->Add(xText);
        } else {
            xText->SetName(newName.c_str());
            xText->customNameLength = static_cast<std::int16_t>(newName.size());
        }
    }

    // -----------------------------------------------------------------------
    // Apply upgrade
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::applyUpgrade(RE::TESForm* item, RE::FormID itemRefId, int newLevel, bool isLeftHand) {
        if (!item) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        // Verify weapon is still equipped and valid
        auto equippedObj = player->GetEquippedObject(isLeftHand);
        if (!equippedObj || equippedObj != item || (itemRefId != 0 && equippedObj->GetFormID() != itemRefId)) {
            logger::warn("applyUpgrade: Equipped weapon changed or unequipped before applying upgrade. Aborting.");
            RE::DebugNotification("Upgrade cancelled: Equipped weapon was unequipped or changed.");
            return;
        }

        RE::ExtraDataList* wornList = getWornExtraList(player, item, isLeftHand);
        if (!wornList) {
            logger::warn("applyUpgrade: wornList is null. Aborting.");
            RE::DebugNotification("Upgrade cancelled: Weapon extra data not found.");
            return;
        }

        auto* weapon = item->As<RE::TESObjectWEAP>();
        const char* rawName = item->GetName();
        std::string itemNameStr = (rawName && rawName[0]) ? rawName : "Weapon";

        // Determine upgrade success using modern mt19937 PRNG
        int currentLevel = newLevel - 1;
        float chance = successChance(currentLevel);

        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float roll = dist(rng);
        bool success = (roll <= chance);

        int finalLevel = success ? newLevel : std::max(0, currentLevel - 1);

        // Set Name
        setExtraTextDisplayData(wornList, item, finalLevel);

        // Set Damage via ExtraHealth
        if (weapon) {
            auto* xHealth = wornList->GetByType<RE::ExtraHealth>();
            if (!xHealth) {
                xHealth = new RE::ExtraHealth();
                xHealth->health = 1.0f;
                wornList->Add(xHealth);
            }

            float baseDmg = static_cast<float>(weapon->attackDamage);
            if (baseDmg <= 0.0f) baseDmg = 1.0f; // safety

            float newBonus = static_cast<float>((finalLevel * (finalLevel + 1)) / 2);
            float oldBonus = static_cast<float>((currentLevel * (currentLevel + 1)) / 2);

            // Preserve vanilla tempering base
            float currentHealth = xHealth->health;
            float vanillaBaseHealth = currentHealth - (oldBonus / baseDmg);
            if (vanillaBaseHealth < 1.0f) vanillaBaseHealth = 1.0f;

            xHealth->health = vanillaBaseHealth + (newBonus / baseDmg);
            
            // Set temperFactor to ensure the engine displays the correct quality tier dynamically!
            auto* xText = wornList->GetByType<RE::ExtraTextDisplayData>();
            if (xText) {
                xText->temperFactor = xHealth->health;
            }
        }

        if (success) {
            logger::info("Item '{}' SUCCESS +{}.", itemNameStr, newLevel);
            std::string msg = std::format("{} upgraded to +{}!", itemNameStr, newLevel);
            RE::DebugNotification(msg.c_str());
        } else {
            logger::info("Item '{}' upgraded FAILURE trying +{}. De-leveled to +{}", itemNameStr, newLevel, finalLevel);
            std::string msg = std::format("{} upgrade failed! Level decreased to +{}.", itemNameStr, finalLevel);
            RE::DebugNotification(msg.c_str());
        }

        refreshGlow();
    }

    // -----------------------------------------------------------------------
    // Glow colour table (R, G, B, baseMult)
    // -----------------------------------------------------------------------

    static std::tuple<float, float, float, float> glowColorForLevel(int level) {
        auto& config = Config::getInstance();
        if (level <= 0) return {1.0f, 1.0f, 1.0f, 1.0f};
        
        int index = level - 1;
        if (index >= static_cast<int>(config.glowColors.size())) {
            return config.glowColors.empty() ? std::make_tuple(1.0f, 1.0f, 1.0f, 1.0f) : config.glowColors.back();
        }
        return config.glowColors[index];
    }

    static float pulseFrequency(int level) {
        // +1=0.3Hz, +2=0.4Hz, +3=0.5Hz ... +9=1.1Hz
        return 0.3f + (level - 1) * 0.1f;
    }

    static std::pair<float, float> pulseRange(int level) {
        float peakMult = 5.5f;
        if (level < 7) {
            peakMult = std::get<3>(glowColorForLevel(level));
        } else if (level == 7) {
            peakMult = 5.5f;
        } else if (level == 8) {
            peakMult = 6.5f;
        } else {
            peakMult = 8.0f;
        }
        float minFactor = 0.05f + (level - 1) * 0.025f;
        float minMult = peakMult * minFactor;
        return { minMult, peakMult };
    }

    struct RGB { float r, g, b; };

    static RGB lerpColor(const RGB& c1, const RGB& c2, float t) {
        return {
            c1.r + (c2.r - c1.r) * t,
            c1.g + (c2.g - c1.g) * t,
            c1.b + (c2.b - c1.b) * t
        };
    }

    static std::tuple<float, float, float> dynamicGlowColor(int level, float time) {
        // Defines the palettes for +7, +8, +9
        std::vector<RGB> palette;
        float speed = 1.0f;

        if (level == 7) {
            // Poison/Acid: Bright Lime Green -> Dark Brown/Rust -> Bright Yellow
            palette = { {0.1f, 1.0f, 0.1f}, {0.4f, 0.15f, 0.0f}, {1.0f, 1.0f, 0.0f} };
            speed = 0.8f;
        } else if (level == 8) {
            // Spark/Electric: Bright Cyan -> Deep Dark Blue -> Bright Purple
            palette = { {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.9f}, {0.8f, 0.0f, 1.0f} };
            speed = 1.0f;
        } else {
            // Fire/Legendary: Bright Yellow -> Deep Red -> Bright Orange
            palette = { {1.0f, 1.0f, 0.0f}, {0.9f, 0.0f, 0.0f}, {1.0f, 0.4f, 0.0f} };
            speed = 1.2f + (level - 9) * 0.2f; // Gets faster at +10, +11...
        }

        // Calculate continuous index and fraction
        float t = std::fmod(time * speed, static_cast<float>(palette.size()));
        int idx1 = static_cast<int>(t);
        int idx2 = (idx1 + 1) % palette.size();
        float frac = t - idx1;

        RGB c = lerpColor(palette[idx1], palette[idx2], frac);
        return {c.r, c.g, c.b};
    }

    void WeaponUpgradeSystem::applyEffectGlow(RE::NiAVObject* node, int level, float t, int depth) {
        if (!node || depth > 20) return;

        float r, g, b, baseMult;
        std::tie(r, g, b, baseMult) = glowColorForLevel(level);

        // Dynamic Color Shift for +7 and above
        if (level >= 7) {
            auto [dr, dg, db] = dynamicGlowColor(level, t);
            r = dr;
            g = dg;
            b = db;
        }

        float freq = pulseFrequency(level);
        auto [minM, maxM] = pulseRange(level);
        float sineVal = 0.5f + 0.5f * std::sin(2.0f * 3.14159265f * freq * t);
        float emissiveMult = minM + (maxM - minM) * sineVal;

        // Apply a tiny random color shift for static colors, but skip it for dynamic colors
        float fr = r, fg = g, fb = b;
        if (level < 7) {
            float shiftAmt = std::sin(t * (1.0f + level * 0.15f)) * (0.04f + level * 0.006f);
            fr = std::clamp(r + shiftAmt, 0.0f, 1.0f);
            fg = std::clamp(g + shiftAmt * 0.5f, 0.0f, 1.0f);
            fb = std::clamp(b - shiftAmt * 0.3f, 0.0f, 1.0f);
        }

        if (auto geom = node->AsGeometry()) {
            auto& props = geom->GetGeometryRuntimeData().properties;
            for (auto& p : props) {
                if (!p) continue;
                if (p->GetRTTI() && std::string_view(p->GetRTTI()->name).find("BSLightingShaderProperty") != std::string_view::npos) {
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
                if (p->GetRTTI() && std::string_view(p->GetRTTI()->name).find("BSLightingShaderProperty") != std::string_view::npos) {
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
        auto glowWeaponNodes = [&](RE::NiAVObject* root, int level) {
            if (!root) return;
            for (const char* name : { "WEAPON", "Bow", "WeaponBow" }) {
                auto* n = root->GetObjectByName(name);
                if (n) applyEffectGlow(n, level, t);
            }
        };

        auto glowShieldNodes = [&](RE::NiAVObject* root, int level) {
            if (!root) return;
            for (const char* name : { "WEAPONL", "SHIELD" }) {
                auto* n = root->GetObjectByName(name);
                if (n) applyEffectGlow(n, level, t);
            }
        };

        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) return;

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        static RE::TESFaction* followerFaction = nullptr;
        if (!followerFaction && dataHandler) {
            // PlayerFollowerFaction is 0x5C84D in Skyrim.esm
            followerFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x5C84D);
        }

        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (!actor || actor->IsPlayerRef()) continue;
            if (!actor->Is3DLoaded()) continue;

            // Check if teammate or in follower faction
            bool isFollower = actor->IsPlayerTeammate();
            if (!isFollower && followerFaction) {
                isFollower = actor->IsInFaction(followerFaction);
            }

            if (!isFollower) continue;

            auto* root = actor->Get3D(false);
            if (!root) continue;

            // Right hand
            auto* equippedR = actor->GetEquippedObject(false);
            if (equippedR) {
                int lvR = parseLevelFromExtraList(getWornExtraList(actor.get(), equippedR, false));
                if (lvR > 0) {
                    glowWeaponNodes(root, lvR);
                    if (auto* w = equippedR->As<RE::TESObjectWEAP>()) {
                        if (w->IsBow() || w->IsCrossbow()) {
                            glowShieldNodes(root, lvR); // Bow geometry is often on WEAPONL when drawn
                        }
                    }
                }
            }

            // Left hand weapon only (dual wielding)
            auto* equippedL = actor->GetEquippedObject(true);
            if (equippedL && equippedL->IsWeapon()) {
                int lvL = parseLevelFromExtraList(getWornExtraList(actor.get(), equippedL, true));
                if (lvL > 0) glowShieldNodes(root, lvL);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Glow animation — background thread drives time, main thread paints
    // -----------------------------------------------------------------------

    void WeaponUpgradeSystem::setGlowEnabled(bool enabled) {
        glowEnabled_.store(enabled, std::memory_order_relaxed);
        if (enabled) {
            triggerDelayedRefresh();
        } else {
            // Clear current glow on player
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player && player->Is3DLoaded()) {
                std::array<RE::NiAVObject*, 2> roots = { player->Get3D(false), player->Get3D(true) };
                for (auto* root : roots) {
                    if (root) clearEffectGlow(root);
                }
            }
        }
    }

    void WeaponUpgradeSystem::triggerDelayedRefresh() {
        if (!glowEnabled_.load(std::memory_order_relaxed)) return;

        uint64_t currentReq = ++refreshGen_;
        std::thread([this, currentReq]() {
            for (int delay : { 200, 300, 500 }) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                if (refreshGen_.load(std::memory_order_relaxed) != currentReq || !animRunning_.load(std::memory_order_relaxed)) {
                    return;
                }
                SKSE::GetTaskInterface()->AddTask([this, currentReq]() {
                    if (refreshGen_.load(std::memory_order_relaxed) == currentReq) {
                        refreshGlow();
                    }
                });
            }
        }).detach();
    }

    // Called from background thread via AddTask — runs on the main game thread.
    void WeaponUpgradeSystem::updateGlowAnimation() {
        if (!glowEnabled_.load(std::memory_order_relaxed)) return;

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
                for (const char* name : { "WEAPON", "Bow", "WeaponBow", "WeaponSword", "WeaponDagger", "WeaponMace", "WeaponAxe", "WeaponBack", "WeaponStaff" }) {
                    auto* n = root->GetObjectByName(name);
                    if (n) applyEffectGlow(n, lvR, t);
                }
            }
            
            if (lvL > 0) {
                for (const char* name : { "WEAPONL", "SHIELD", "Shield", "ShieldNode", "WeaponShield", "NPC L Forearm [LLar]", "WeaponSwordLeft", "WeaponDaggerLeft", "WeaponMaceLeft", "WeaponAxeLeft", "WeaponBackLeft", "WeaponStaffLeft" }) {
                    auto* n = root->GetObjectByName(name);
                    if (n) applyEffectGlow(n, lvL, t);
                }
            }
        }

        // Follower glow: throttled to run only every 10 ticks (~3 times a second at 30fps).
        int throttle = followerGlowThrottle_.fetch_add(1, std::memory_order_relaxed);
        if (throttle % 10 == 0) {
            applyFollowerGlows(t);
        }
    }

    void WeaponUpgradeSystem::startGlowLoop() {
        // Stop any existing thread before starting a new one
        stopGlowLoop();

        glowEnabled_.store(Config::getInstance().enableGlow, std::memory_order_relaxed);
        animRunning_.store(true, std::memory_order_relaxed);
        glowTime_.store(0.0f, std::memory_order_relaxed);

        uint64_t currentGen = ++generation_;

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

        // Dedicated managed animation thread with generation token
        animThread_ = std::thread([this, currentGen]() {
            SKSE::log::info("Glow animation thread started (gen {}).", currentGen);
            auto lastTime = std::chrono::steady_clock::now();

            while (animRunning_.load(std::memory_order_relaxed) && generation_.load(std::memory_order_relaxed) == currentGen) {
                auto   now = std::chrono::steady_clock::now();
                float  dt  = std::chrono::duration<float>(now - lastTime).count();
                lastTime   = now;

                // Clamp dt to avoid huge jumps after hitching
                dt = std::min(dt, 0.1f);

                // Advance time accumulator
                float t = glowTime_.load(std::memory_order_relaxed) + dt;
                glowTime_.store(t, std::memory_order_relaxed);

                if (glowEnabled_.load(std::memory_order_relaxed)) {
                    SKSE::GetTaskInterface()->AddTask([this, currentGen]() {
                        if (generation_.load(std::memory_order_relaxed) == currentGen) {
                            updateGlowAnimation();
                        }
                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
            }
            SKSE::log::info("Glow animation thread stopped (gen {}).", currentGen);
        });

        // Initial glow application
        triggerDelayedRefresh();
    }

    void WeaponUpgradeSystem::stopGlowLoop() {
        animRunning_.store(false, std::memory_order_relaxed);
        glowEnabled_.store(false, std::memory_order_relaxed);
        generation_++;
        refreshGen_++;

        if (animThread_.joinable()) {
            animThread_.join();
            SKSE::log::info("Glow animation thread joined successfully.");
        }

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
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!a_event || !a_event->actor || a_event->actor.get() != player)
            return RE::BSEventNotifyControl::kContinue;

        auto* form   = RE::TESForm::LookupByID(a_event->baseObject);
        auto* weapon = form ? form->As<RE::TESObjectWEAP>() : nullptr;

        if (weapon) {
            triggerDelayedRefresh();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl WeaponUpgradeSystem::ProcessEvent(
        const SKSE::CameraEvent* a_event,
        RE::BSTEventSource<SKSE::CameraEvent>*) 
    {
        if (!glowEnabled_.load(std::memory_order_relaxed)) return RE::BSEventNotifyControl::kContinue;

        if (a_event) {
            triggerDelayedRefresh();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl WeaponUpgradeSystem::ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) 
    {
        if (!glowEnabled_.load(std::memory_order_relaxed)) return RE::BSEventNotifyControl::kContinue;

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
        if (!glowEnabled_.load(std::memory_order_relaxed)) return;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->Is3DLoaded()) return;

        int levelR = 0;
        int levelL = 0;
        RE::FormID formR = 0;
        RE::FormID formL = 0;
        try {
            if (auto equippedR = player->GetEquippedObject(false)) {
                formR = equippedR->GetFormID();
                auto wornList = getWornExtraList(player, equippedR, false);
                levelR = parseLevelFromExtraList(wornList);
                
                SKSE::log::trace("refreshGlow: Right hand level is {}", levelR);
                if (auto* w = equippedR->As<RE::TESObjectWEAP>()) {
                    if (w->IsBow() || w->IsCrossbow()) {
                        levelL = levelR; // Bow geometry is often on WEAPONL when drawn
                        formL = formR;
                        SKSE::log::trace("refreshGlow: Bow/Crossbow detected, setting levelL = {}", levelL);
                    }
                }
            }
        } catch (...) { logger::error("Exception reading right-hand in refreshGlow"); }

        try {
            if (auto equippedL = player->GetEquippedObject(true)) {
                formL = equippedL->GetFormID();
                auto wornList = getWornExtraList(player, equippedL, true);
                int parsedL = parseLevelFromExtraList(wornList);
                if (parsedL > 0) levelL = parsedL;
            }
        } catch (...) { logger::error("Exception reading left-hand in refreshGlow"); }

        SKSE::log::trace("refreshGlow() - LvR={}, LvL={}", levelR, levelL);

        // Cache levels so the animation thread knows what to animate
        lastLevelR_.store(levelR, std::memory_order_relaxed);
        lastLevelL_.store(levelL, std::memory_order_relaxed);
        lastItemR_.store(formR, std::memory_order_relaxed);
        lastItemL_.store(formL, std::memory_order_relaxed);

        float t = glowTime_.load(std::memory_order_relaxed);

        // Apply/clear initial glow on both views
        std::array<RE::NiAVObject*, 2> roots = { player->Get3D(false), player->Get3D(true) };
        for (auto* root : roots) {
            if (!root) continue;

            for (const char* name : { "WEAPON", "Bow", "WeaponBow", "WeaponSword", "WeaponDagger", "WeaponMace", "WeaponAxe", "WeaponBack", "WeaponStaff" }) {
                auto* n = root->GetObjectByName(name);
                if (n) {
                    if (levelR > 0) applyEffectGlow(n, levelR, t);
                    else            clearEffectGlow(n);
                }
            }

            for (const char* name : { "WEAPONL", "SHIELD", "Shield", "ShieldNode", "WeaponShield", "NPC L Forearm [LLar]", "WeaponSwordLeft", "WeaponDaggerLeft", "WeaponMaceLeft", "WeaponAxeLeft", "WeaponBackLeft", "WeaponStaffLeft" }) {
                auto* n = root->GetObjectByName(name);
                if (n) {
                    if (levelL > 0) applyEffectGlow(n, levelL, t);
                    else            clearEffectGlow(n);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // HandSelectionMenuCallback
    // -----------------------------------------------------------------------

    void HandSelectionMenuCallback::Run(RE::IMessageBoxCallback::Message a_button) {
        WeaponUpgradeSystem::getInstance().setMenuOpen(false);

        int btnIndex = static_cast<int>(a_button);
        SKSE::log::info("HandSelectionMenuCallback::Run triggered with button index: {}", btnIndex);

        if (btnIndex == 0 || btnIndex == 4) {
            // Right Hand selected
            SKSE::GetTaskInterface()->AddTask([this]() {
                WeaponUpgradeSystem::getInstance().processUpgradeCheck(itemR_, false);
            });
        } else if (btnIndex == 1 || btnIndex == 5) {
            // Left Hand selected
            SKSE::GetTaskInterface()->AddTask([this]() {
                WeaponUpgradeSystem::getInstance().processUpgradeCheck(itemL_, true);
            });
        } else {
            logger::info("Hand selection cancelled by player.");
        }
    }

    // -----------------------------------------------------------------------
    // UpgradeMenuCallback
    // -----------------------------------------------------------------------

    void UpgradeMenuCallback::Run(RE::IMessageBoxCallback::Message a_button) {
        WeaponUpgradeSystem::getInstance().setMenuOpen(false);

        int btnIndex = static_cast<int>(a_button);
        SKSE::log::info("UpgradeMenuCallback::Run triggered with button index: {}", btnIndex);

        if (btnIndex == 0 || btnIndex == 4) {
            SKSE::log::info("Player selected Upgrade. Validating weapon and gold...");

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            // Pre-validation: ensure the weapon is still equipped before deducting gold
            auto currentEquipped = player->GetEquippedObject(isLeftHand_);
            if (!currentEquipped || currentEquipped != item_ || (weaponRefId_ != 0 && currentEquipped->GetFormID() != weaponRefId_)) {
                logger::warn("UpgradeMenuCallback: Weapon was unequipped or swapped before confirming. Cancelling.");
                RE::DebugNotification("Upgrade cancelled: Weapon was unequipped or changed.");
                return;
            }

            RE::ExtraDataList* wornList = getWornExtraList(player, item_, isLeftHand_);
            if (!wornList) {
                logger::warn("UpgradeMenuCallback: wornList is null. Cancelling.");
                RE::DebugNotification("Upgrade cancelled: Weapon extra data not found.");
                return;
            }

            int cost = upgradeCost(currentLevel_);

            auto inv = player->GetInventory([](const RE::TESBoundObject& obj) {
                return obj.IsGold();
            });

            int goldCount = 0;
            for (auto& [form, data] : inv) {
                goldCount += data.first;
            }

            if (goldCount < cost) {
                std::string msg = std::format("Not enough gold! Required: {} gold (You have: {})", cost, goldCount);
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
            WeaponUpgradeSystem::getInstance().applyUpgrade(item_, weaponRefId_, newLevel, isLeftHand_);

        } else {
            logger::info("Upgrade cancelled by player.");
        }
    }

}  // namespace plugin
