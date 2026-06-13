#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <Windows.h>

namespace plugin {

    /**
     * Simple Config Class that parses the INI file line by line
     * without needing external library dependencies.
     */
    class Config {
    public:
        static Config& getInstance() {
            static Config instance;
            return instance;
        }

        // --- Config settings ---
        uint32_t upgradeKey = 0x26;           // Default: DIK_L = 0x26 (L)
        bool enableGlow = true;               // Glow feature toggle
        int goldCostMultiplier = 100;         // (currentLevel + 1) * multiplier
        int maxUpgradeLevel = 9;              // Maximum upgrade level

        // Success chances for levels 0 to 8 (+1 to +9)
        std::vector<float> successChances = {
            1.00f,  // +1 (currentLevel 0)
            0.90f,  // +2 (currentLevel 1)
            0.80f,  // +3 (currentLevel 2)
            0.70f,  // +4 (currentLevel 3)
            0.60f,  // +5 (currentLevel 4)
            0.50f,  // +6 (currentLevel 5)
            0.40f,  // +7 (currentLevel 6)
            0.30f,  // +8 (currentLevel 7)
            0.25f   // +9 (currentLevel 8)
        };

        void load() {
            std::filesystem::path iniPath = "Data/SKSE/Plugins/WeaponPlusSystem.ini";
            
            // If file doesn't exist, generate default
            if (!std::filesystem::exists(iniPath)) {
                std::filesystem::create_directories(iniPath.parent_path());
                saveDefault(iniPath);
                return;
            }

            std::ifstream file(iniPath);
            if (!file.is_open()) {
                SKSE::log::error("Config::load() - Failed to open ini file!");
                return;
            }

            SKSE::log::info("Config::load() - Reading settings from: {}", iniPath.string());
            std::string line;
            while (std::getline(file, line)) {
                // Remove whitespaces and comments
                line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
                if (line.empty() || line[0] == ';' || line[0] == '#') continue;

                auto delimiterPos = line.find('=');
                if (delimiterPos == std::string::npos) continue;

                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);

                if (key == "UpgradeKey") {
                    try {
                        upgradeKey = std::stoul(value, nullptr, 16); // Hex read
                    } catch (...) {
                        try { upgradeKey = std::stoul(value); } catch (...) {}
                    }
                } else if (key == "EnableGlow") {
                    enableGlow = (value == "true" || value == "1");
                } else if (key == "GoldCostMultiplier") {
                    try { goldCostMultiplier = std::stoi(value); } catch (...) {}
                } else if (key == "MaxUpgradeLevel") {
                    try { maxUpgradeLevel = std::stoi(value); } catch (...) {}
                } else if (key.rfind("ChanceLevel", 0) == 0) { // starts with ChanceLevel
                    try {
                        int index = std::stoi(key.substr(11));
                        float chance = std::stof(value);
                        if (index >= 0 && index < 10) {
                            if (index >= static_cast<int>(successChances.size())) {
                                successChances.resize(index + 1, 0.25f);
                            }
                            successChances[index] = chance;
                        }
                    } catch (...) {}
                }
            }

            SKSE::log::info("Config Loaded: UpgradeKey=0x{:X}, Glow={}, Multiplier={}, MaxLevel={}",
                upgradeKey, enableGlow, goldCostMultiplier, maxUpgradeLevel);
        }

    private:
        Config() = default;

        void saveDefault(const std::filesystem::path& path) {
            std::ofstream file(path);
            if (!file.is_open()) return;

            file << "; Weapon Plus System Settings\n\n";
            file << "[General]\n";
            file << "; DirectInput Keyboard Scan Code (e.g. 0x26 = L, 0x1E = A, 0x2D = X)\n";
            file << "UpgradeKey = 0x26\n\n";
            file << "; Toggle weapon/shield glow effects (true/false)\n";
            file << "EnableGlow = true\n\n";
            file << "; Cost of upgrading: (CurrentLevel + 1) * GoldCostMultiplier\n";
            file << "GoldCostMultiplier = 100\n\n";
            file << "; Maximum level a weapon/shield can be upgraded to (Default: 9)\n";
            file << "MaxUpgradeLevel = 9\n\n";
            file << "[SuccessChances]\n";
            file << "; Probability rates between 0.0 (0%) and 1.0 (100%)\n";
            file << "ChanceLevel0 = 1.00   ; Attempting +1\n";
            file << "ChanceLevel1 = 0.90   ; Attempting +2\n";
            file << "ChanceLevel2 = 0.80   ; Attempting +3\n";
            file << "ChanceLevel3 = 0.70   ; Attempting +4\n";
            file << "ChanceLevel4 = 0.60   ; Attempting +5\n";
            file << "ChanceLevel5 = 0.50   ; Attempting +6\n";
            file << "ChanceLevel6 = 0.40   ; Attempting +7\n";
            file << "ChanceLevel7 = 0.30   ; Attempting +8\n";
            file << "ChanceLevel8 = 0.25   ; Attempting +9\n";

            SKSE::log::info("Generated default Config at: {}", path.string());
        }
    };

}  // namespace plugin
