#pragma once

namespace plugin {

    // Max upgrade level
    inline constexpr int kMaxPlusLevel = 9;

    // Gold cost per level: level 1 costs 100, level 2 costs 200, ..., level N costs N*100
    inline int upgradeCost(int currentLevel) {
        return (currentLevel + 1) * 100;
    }

    // Success chance for upgrading from currentLevel to currentLevel + 1:
    // currentLevel 0 (+1 attempt) = 100% (1.0f)
    // currentLevel 8 (+9 attempt) = 25% (0.25f)
    inline float successChance(int currentLevel) {
        if (currentLevel <= 0) return 1.0f;
        if (currentLevel >= 8) return 0.25f;
        // Linear step: 100% to 25% over 8 steps -> 75% total span -> 9.375% per level reduction
        return 1.00f - (currentLevel * 0.09375f);
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
            if (lvl >= kMaxPlusLevel) return false;
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
