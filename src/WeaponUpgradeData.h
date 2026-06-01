#pragma once

namespace plugin {

    // Max upgrade level
    inline constexpr int kMaxPlusLevel = 9;

    // Gold cost per level: level 1 costs 100, level 2 costs 200, ..., level N costs N*100
    inline int upgradeCost(int currentLevel) {
        return (currentLevel + 1) * 100;
    }

    // Damage multiplier bonus per level: +10% per level
    inline float damageBonus(int level) {
        return 1.0f + (level * 0.10f);
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
