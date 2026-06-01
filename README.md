# Weapon Plus System (SKSE Plugin)

[TR] Türkçe açıklama aşağıdadır.

A premium, performance-optimized **Skyrim Special Edition (v1.6.1170)** SKSE C++ plugin that introduces a comprehensive weapon upgrade system. Players can level up their weapons by interacting with blacksmith NPCs, gaining both dynamic damage scaling and beautiful, tier-based visual glow effects.

---

## English Version

### Features
* **Blacksmith NPC Interaction**: Point your crosshair at any blacksmith NPC and press the **L** key to open the interactive upgrade menu.
* **Tier-Based Upgrades (Up to +9)**: Upgrade your equipped weapons up to +9. Each level requires gold and increases the base weapon damage dynamically (+10% damage bonus per level).
* **Dynamic Visual Glow Effects**: Upgraded weapons receive an emissive magical glow that scales in intensity and changes color based on the upgrade level:
  * **+1**: White
  * **+2**: Light Blue
  * **+3**: Deep Blue
  * **+4**: Cyan
  * **+5**: Green
  * **+6**: Yellow
  * **+7**: Orange
  * **+8**: Red-Orange
  * **+9**: Deep Red (Magnum Opus)
* **First-Person & Third-Person Support**: The glowing visual shader is automatically refreshed and perfectly visible in both 1st-person arms and 3rd-person world models.
* **Smart UI & Camera Event Tracking**: Zero performance overhead! The plugin is fully event-driven, listening directly to `TESEquipEvent`, `SKSE::CameraEvent` (1st/3rd person switches), and `RE::MenuOpenCloseEvent` (when closing inventory/favorites/crafting menus) to safely re-apply glows asynchronously without standard heavy frame loops.
* **SKSE Cosave Serialization**: Upgrade levels are fully persistent and cleanly saved and loaded inside your standard Skyrim save files.
* **Community Shaders Compatibility**: Built with custom direct material emittance overrides bypasses the standard BSEffectShader data structure, ensuring 100% crash-free stability when running *Community Shaders* (including Metals and Dynamic Cubemaps).

### How to Use
1. Equip the weapon you want to upgrade in either hand.
2. Approach any blacksmith NPC and point your crosshair at them.
3. Press **L** to open the upgrade menu.
4. If you have enough gold, confirm the upgrade!
5. **Remember to save your game (F5 or manual save) after upgrading so your weapon upgrades persist!**

---

## Türkçe Versiyon

Skyrim Special Edition için geliştirilmiş, performans odaklı ve son teknoloji bir **SKSE C++ eklentisidir**. Oyuncuların demirci NPC'ler ile etkileşime girerek silahlarını seviyelendirmelerini, hasarlarını artırmalarını ve yükseltme seviyesine göre harika görsel parlama efektleri kazanmalarını sağlar.

### Özellikler
* **Demirci Etkileşimi**: Hedef göstergenizi (crosshair) herhangi bir demirci NPC'ye doğrultup **L** tuşuna basarak yükseltme menüsünü açabilirsiniz.
* **9 Aşamalı Yükseltme (+9'a Kadar)**: Kuşanmış olduğunuz silahları altın karşılığında +9 seviyeye kadar yükseltebilirsiniz. Her seviye silahın taban hasarını dinamik olarak artırır (seviye başına +%10 hasar bonusu).
* **Dinamik Görsel Parlama Efektleri**: Yükseltilen silahlar, seviyesine göre renk ve yoğunluk değiştiren büyülü bir ışıma kazanır:
  * **+1**: Beyaz
  * **+2**: Açık Mavi
  * **+3**: Koyu Mavi
  * **+4**: Turkuaz / Camgöbeği
  * **+5**: Yeşil
  * **+6**: Sarı
  * **+7**: Turuncu
  * **+8**: Kırmızı-Turuncu
  * **+9**: Derin Kırmızı (Efsanevi Seviye)
* **1. Şahıs & 3. Şahıs Kamera Desteği**: Silah parlamaları hem 3. şahıs (dünya) görünümünde hem de 1. şahıs (kamera kolları) görünümünde otomatik olarak yenilenir ve mükemmel çalışır.
* **Akıllı Etkinlik Takibi**: Sıfır performans kaybı! Ağır per-frame (kare başı) döngüler yerine; kuşanma olaylarını (`TESEquipEvent`), kamera değişimlerini (`SKSE::CameraEvent`) ve envanter/kısayol menü kapanışlarını (`RE::MenuOpenCloseEvent`) dinleyerek parlamayı tamamen asenkron ve güvenli şekilde günceller.
* **SKSE Cosave Kayıt Sistemi**: Silahlarınızın seviyeleri tamamen kalıcıdır; oyunu kaydettiğinizde otomatik olarak `.ess / .cosave` kayıt dosyalarınıza yazılır ve oyunu tekrar açtığınızda sorunsuz bir şekilde yüklenir.
* **Community Shaders Uyumluluğu**: *Community Shaders* (Metals ve Dynamic Cubemaps dahil) kurulu sistemlerde çökmeye neden olan klasik shader yapısı yerine doğrudan materyal ışıması (`BSLightingShaderProperty`) modifikasyonları kullanılarak %100 kararlılık sağlanmıştır.

### Nasıl Kullanılır?
1. Yükseltmek istediğiniz silahı elinize kuşanın.
2. Herhangi bir demirci NPC'ye yaklaşın ve hedef göstergenizi ona doğrultun.
3. **L** tuşuna basarak yükseltme ekranını açın.
4. Yeterli altınınız varsa yükseltmeyi onaylayın!
5. **Silahınızı yükselttikten sonra oyunu kaydetmeyi (F5 veya normal kayıt) unutmayın; böylece yükseltmeleriniz kalıcı olarak kaydedilecektir!**

---

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır. See [LICENSE](file:///c:/Users/pc/Desktop/projeler/Weapon%20Plus%20System/LICENSE.md) for details.