#pragma once

#include "Config.h"

namespace plugin {

    // Gold cost per level: level 1 costs (0+1)*multiplier, level 2 costs (1+1)*multiplier...
    inline int upgradeCost(int currentLevel) {
        return (currentLevel + 1) * Config::getInstance().goldCostMultiplier;
    }

    // Success chance for upgrading from currentLevel to currentLevel + 1
    inline float successChance(int currentLevel) {
        auto& config = Config::getInstance();
        if (currentLevel < 0) return 1.0f;
        if (currentLevel >= static_cast<int>(config.successChances.size())) {
            return config.successChances.empty() ? 0.25f : config.successChances.back();
        }
        return config.successChances[currentLevel];
    }


    /**
     * WeaponUpgradeData - Singleton that stores the + level for each weapon (by FormID).
     * Also persists data via SKSE co-save (serialized in WeaponPlusSystem.cosave).
     */
    class WeaponUpgradeData {
    public:
        static WeaponUpgradeData& getInstance() {
            static WeaponUpgradeData instance;
            return instance;
        }

        WeaponUpgradeData(const WeaponUpgradeData&) = delete;
        WeaponUpgradeData& operator=(const WeaponUpgradeData&) = delete;

        // Returns the current + level for the given unique objectFormID
        // We use the unique RefID (RE::FormID of the ObjectReference) as key.
        int getLevel(RE::FormID refId) const {
            auto it = upgradeMap_.find(refId);
            return (it != upgradeMap_.end()) ? it->second : 0;
        }

        // Sets the + level
        void setLevel(RE::FormID refId, int level) {
            upgradeMap_[refId] = level;
        }

        // Increment level (returns false if already at max)
        bool incrementLevel(RE::FormID refId) {
            int lvl = getLevel(refId);
            if (lvl >= Config::getInstance().maxUpgradeLevel) return false;
            upgradeMap_[refId] = lvl + 1;
            return true;
        }

        void clear() { upgradeMap_.clear(); }

        const std::unordered_map<RE::FormID, int>& getAll() const { return upgradeMap_; }

    private:
        WeaponUpgradeData() = default;
        std::unordered_map<RE::FormID, int> upgradeMap_;
    };

}  // namespace plugin
