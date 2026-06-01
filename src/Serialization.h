#pragma once
#include "WeaponUpgradeData.h"

namespace plugin {

    /**
     * Serialization - persists WeaponUpgradeData across save/load via SKSE co-saves.
     */
    class Serialization {
    public:
        // Unique identifier for our plugin's records (4 ASCII chars)
        static constexpr uint32_t kSerializationKey  = 'WPLS';  // Weapon PLus System
        static constexpr uint32_t kVersion            = 1;
        static constexpr uint32_t kRecordType         = 'DATA';

        static void onSave(SKSE::SerializationInterface* intf) {
            SKSE::log::info("Serialization::onSave()");

            if (!intf->OpenRecord(kRecordType, kVersion)) {
                SKSE::log::error("Failed to open record for saving.");
                return;
            }

            auto& map = WeaponUpgradeData::getInstance().getAll();
            uint32_t count = static_cast<uint32_t>(map.size());
            intf->WriteRecordData(&count, sizeof(count));

            for (auto& [formId, level] : map) {
                intf->WriteRecordData(&formId, sizeof(formId));
                intf->WriteRecordData(&level, sizeof(level));
            }

            SKSE::log::info("Saved {} weapon upgrade entries.", count);
        }

        static void onLoad(SKSE::SerializationInterface* intf) {
            SKSE::log::info("Serialization::onLoad()");
            WeaponUpgradeData::getInstance().clear();

            uint32_t type = 0, version = 0, length = 0;
            while (intf->GetNextRecordInfo(type, version, length)) {
                if (type == kRecordType) {
                    uint32_t count = 0;
                    intf->ReadRecordData(&count, sizeof(count));

                    for (uint32_t i = 0; i < count; ++i) {
                        RE::FormID formId = 0;
                        int        level  = 0;
                        intf->ReadRecordData(&formId, sizeof(formId));
                        intf->ReadRecordData(&level,  sizeof(level));

                        // Resolve FormID to handle load order changes
                        RE::FormID resolvedId = 0;
                        if (intf->ResolveFormID(formId, resolvedId)) {
                            WeaponUpgradeData::getInstance().setLevel(resolvedId, level);
                        }
                    }

                    SKSE::log::info("Loaded {} weapon upgrade entries.", count);
                }
            }
        }

        static void onRevert(SKSE::SerializationInterface*) {
            SKSE::log::info("Serialization::onRevert() - clearing upgrade data.");
            WeaponUpgradeData::getInstance().clear();
        }

        static void install() {
            auto* intf = SKSE::GetSerializationInterface();
            if (!intf) {
                SKSE::log::error("SKSE Serialization interface not available!");
                return;
            }

            intf->SetUniqueID(kSerializationKey);
            intf->SetSaveCallback(onSave);
            intf->SetLoadCallback(onLoad);
            intf->SetRevertCallback(onRevert);

            SKSE::log::info("Serialization installed.");
        }
    };

}  // namespace plugin
