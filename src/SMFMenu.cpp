#include "SMFMenu.h"
#include "SKSEMenuFramework.h"
#include "Config.h"
#include "WeaponUpgradeSystem.h"
#include <string>

namespace ImGui = ImGuiMCP;

namespace plugin {

    static void __stdcall RenderGeneralSettings() {
        auto& config = Config::getInstance();

        ImGui::Text("Weapon Plus System - General Configuration");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Checkbox("Enable Weapon Glow", &config.enableGlow)) {
            WeaponUpgradeSystem::getInstance().setGlowEnabled(config.enableGlow);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable or disable glowing visual effects on upgraded weapons.");
        }

        ImGui::SliderInt("Gold Cost Multiplier", &config.goldCostMultiplier, 1, 1000);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Formula: (Current Level + 1) * Multiplier");
        }

        ImGui::SliderInt("Max Upgrade Level", &config.maxUpgradeLevel, 1, 9);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maximum level a weapon can be upgraded to.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings to INI")) {
            config.save();
        }
    }

    static void __stdcall RenderSuccessChances() {
        auto& config = Config::getInstance();

        ImGui::Text("Upgrade Success Chances per Level");
        ImGui::Separator();
        ImGui::Spacing();

        for (size_t i = 0; i < config.successChances.size(); ++i) {
            std::string label = "Attempting +" + std::to_string(i + 1) + " (Level " + std::to_string(i) + " -> +" + std::to_string(i + 1) + ")";
            ImGui::SliderFloat(label.c_str(), &config.successChances[i], 0.0f, 1.0f, "%.2f");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings to INI")) {
            config.save();
        }
    }

    static void __stdcall RenderGlowColors() {
        auto& config = Config::getInstance();

        ImGui::Text("Custom Weapon Glow Colors (+1 to +6)");
        ImGui::Separator();
        ImGui::Spacing();

        for (size_t i = 0; i < 6 && i < config.glowColors.size(); ++i) {
            std::string label = "Glow +" + std::to_string(i + 1);
            auto [r, g, b, mult] = config.glowColors[i];
            float col[4] = { r, g, b, mult };
            if (ImGui::ColorEdit4(label.c_str(), col)) {
                config.glowColors[i] = { col[0], col[1], col[2], col[3] };
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Note: +7, +8, and +9 use dynamic high-contrast RGB LERP transitions.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings to INI")) {
            config.save();
        }
    }

    void RegisterSMFMenu() {
        if (!SKSEMenuFramework::IsInstalled()) {
            SKSE::log::info("SKSE Menu Framework not detected. Skipping in-game menu registration.");
            return;
        }

        SKSEMenuFramework::SetSection("Weapon Plus System");
        SKSEMenuFramework::AddSectionItem("General Settings", RenderGeneralSettings);
        SKSEMenuFramework::AddSectionItem("Success Chances", RenderSuccessChances);
        SKSEMenuFramework::AddSectionItem("Glow Colors", RenderGlowColors);

        SKSE::log::info("Registered Weapon Plus System sections with SKSE Menu Framework.");
    }
}
