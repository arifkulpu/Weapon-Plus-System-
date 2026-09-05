# Weapon Plus System (SKSE Plugin)

[TR] Türkçe açıklama aşağıdadır.

A premium, performance-optimized **Skyrim Special Edition (v1.6.1170)** SKSE C++ plugin that introduces a comprehensive weapon upgrade system. Players can level up their weapons and shields by interacting with blacksmith NPCs, gaining both dynamic damage scaling and beautiful, tier-based animated glow effects — and their **companions** will also glow when wielding upgraded items!

---

## English Version

### Features
* **Blacksmith NPC Interaction**: Point your crosshair at any blacksmith NPC and press the **L** key to open the interactive upgrade menu.
* **Tier-Based Upgrades (Up to +9)**: Upgrade your equipped **weapons and shields** up to +9. Each level requires gold and increases the base weapon damage dynamically by adding flat damage (+1 damage bonus per level: +1 at level 1, +9 at level 9). Shields can also be upgraded for increased glow level.
* **Upgrade Success Rate & Downgrade Risk**: 
  * Upgrading is not guaranteed to succeed! The success rate starts at **100%** for a +1 attempt, and scales down to **25%** for a +9 attempt (linearly scaling down per level).
  * On upgrade failure, the weapon's level **drops by 1 level** (e.g. failing a +5 attempt drops it to +3). Level cannot drop below +0.
* **Dynamic Animated Glow Effects**: Upgraded weapons receive an emissive magical glow that pulses and breathes in real time. Both pulse speed and intensity scale with the upgrade level:
  * **+1**: White — very slow, subtle shimmer (0.3 Hz)
  * **+2**: Light Blue (0.4 Hz)
  * **+3**: Deep Blue (0.5 Hz)
  * **+4**: Cyan (0.6 Hz)
  * **+5**: Green (0.7 Hz)
  * **+6**: Yellow (0.8 Hz)
  * **+7**: Orange (0.9 Hz)
  * **+8**: Red-Orange (1.0 Hz)
  * **+9**: Deep Red — fast, dramatic flash (1.1 Hz)
* **Level-Scaled Pulse Animation**: The glow breathes at a unique speed for each upgrade tier. At +1 the pulse is barely visible (~3.3 second cycle), while at +9 it flashes intensely (~0.9 second cycle). Brightness amplitude also grows with level.
* **First-Person & Third-Person Support**: The glowing visual shader is automatically refreshed and perfectly visible in both 1st-person arms and 3rd-person world models.
* **Smart UI & Camera Event Tracking**: Zero performance overhead! The plugin is fully event-driven, listening directly to `TESEquipEvent`, `SKSE::CameraEvent` (1st/3rd person switches), and `RE::MenuOpenCloseEvent` (when closing inventory/favorites/crafting menus) to safely re-apply glows asynchronously without heavy frame loops.
* **Background Animation Thread**: A dedicated ~30 fps background thread advances the glow timer and dispatches a single update task to the main thread each tick — no recursive scheduling, no hangs.
* **Name & ExtraHealth Persistence System**: Upgrade levels and damage scaling are persistent and sealed directly to the item instance (`ExtraTextDisplayData` and `ExtraHealth`), surviving game saves, loads, and inventory transfers without external save bloat.
* **Community Shaders Compatibility**: Built with custom direct material emittance overrides that bypass the standard BSEffectShader data structure, ensuring 100% crash-free stability when running *Community Shaders* (including Metals and Dynamic Cubemaps).

### Changelog

#### [NEW] Version 1.1.0
* **SKSE Menu Framework (SMF) Integration**: Full in-game configuration menu support! You can now adjust all mod settings (glow toggles, gold cost multiplier, max upgrade level, per-level success chances, and glow colors) in real-time inside Skyrim using SKSE Menu Framework.
* **In-Game Save to INI**: Includes a "Save Settings to INI" button directly inside the menu interface to persist your custom settings across game sessions.

#### Version 1.0.8
* **Improved Dynamic Color Transitions**: The color palettes for +7, +8, and +9 weapons have been redesigned with high-contrast colors so that transitions are clearly visible.
  * **+7 (Poison/Acid)**: Bright Lime Green → Dark Brown/Rust → Bright Yellow
  * **+8 (Electric/Spark)**: Bright Cyan → Deep Dark Blue → Bright Purple
  * **+9 (Fire/Legendary)**: Bright Yellow → Deep Red → Bright Orange

#### [NEW] Version 1.0.6
* **Dynamic Color Transitions (RGB LERP System)**: The colors of +7 and above weapons are no longer static. In addition to the breathing effect, color palettes smoothly transition into one another, creating a fluid visual feast. Colors between +1 and +6 have also been recalibrated to be more harmonious.
* **Name Persistence System**: Weapon upgrade levels (e.g., `+7`) are now sealed to the item's Custom Name. This ensures that the upgrade level is not lost when saving and reloading the game.
* **Code and Performance Optimization**: Unused systems (Particle ArtObject experiments, texture-breaking UV Scrolling mechanics, etc.) have been completely removed, and the SKSE animation loop (30 FPS) has been optimized, making the mod extremely lightweight and FPS-friendly.

#### Version 1.0.3
* **INI Configuration File Support**: Adds a fully customizable `WeaponPlusSystem.ini` config file automatically generated inside `Data/SKSE/Plugins/`. Users can now customize:
  * Key bindings (`UpgradeKey`) using DirectInput Hex codes (Default: `0x26` for **L**).
  * Toggle glow effects (`EnableGlow`).
  * Price scale parameters (`GoldCostMultiplier`).
  * Maximum upgrade bounds (`MaxUpgradeLevel`).
  * Dynamic success probabilities per attempt (`ChanceLevel0` to `ChanceLevel8`).
* **Individual Weapon Upgrades (Tempering/ExtraHealth)**: Upgrading no longer increases base item stats globally. The plugin now uses Skyrim's internal `ExtraHealth` (tempering) data structure to store and scale item stats, modifying **only** the unique weapon instance upgraded.
* **Multi-Version Independent DLL**: Build features updated via CommonLibSSE-NG to support a single independent DLL compatible with Skyrim Special Edition **(v1.5.97)**, Anniversary Edition **(v1.6.1170)**, and **Skyrim VR**.

### How to Use
1. Equip the weapon or shield you want to upgrade.
2. Approach any blacksmith NPC and point your crosshair at them.
3. Press **L** (or your custom configured key in the INI file) to open the upgrade menu.
4. If you have enough gold, check the success chance and confirm the upgrade!
5. **Remember to save your game (F5 or manual save) after upgrading so your weapon upgrades persist!**

---

## Türkçe Versiyon

Skyrim Special Edition için geliştirilmiş, performans odaklı ve son teknoloji bir **SKSE C++ eklentisidir**. Oyuncuların demirci NPC'ler ile etkileşime girerek silah ve kalkanlarını seviyelendirmelerini, hasarlarını artırmalarını ve yükseltme seviyesine göre gerçek zamanlı animasyonlu görsel parlama efektleri kazanmalarını sağlar. Yükseltilmiş eşyalar **takipçilerin** elinde de parlar!

### Özellikler
* **Demirci Etkileşimi**: Hedef göstergenizi (crosshair) herhangi bir demirci NPC'ye doğrultup **L** tuşuna basarak yükseltme menüsünü açabilirsiniz.
* **9 Aşamalı Yükseltme (+9'a Kadar)**: Kuşanmış olduğunuz **silah ve kalkanları** altın karşılığında +9 seviyeye kadar yükseltebilirsiniz. Her seviye silahın taban hasarını sabit (flat) olarak artırır (seviye başına +1 hasar bonusu: +1 seviyede +1 hasar, +9 seviyede +9 hasar). Kalkanlar da aynı şekilde yükseltilebilir ve seviyeye göre parlama kazanır.
* **Başarı Şansı ve Seviye Düşme Riski**:
  * Yükseltme her zaman başarılı olmaz! Başarı şansı +1 denemesinde **%100** iken, +9 denemesinde **%25**'e kadar doğrusal olarak düşer.
  * Başarısız yükseltme girişimlerinde silah **1 seviye geri düşer** (örn. +4'ten +5'e denerken başarısız olunursa silah +3'e geriler). Seviye +0'ın altına düşmez.
* **Dinamik Animasyonlu Parlama Efektleri**: Yükseltilen silahlar, gerçek zamanlı olarak yanıp sönen büyülü bir ışıma kazanır. Yanıp sönme hızı ve parlaklık yoğunluğu yükseltme seviyesiyle birlikte artar:
  * **+1**: Beyaz — çok yavaş, hafif pırıltı (0.3 Hz)
  * **+2**: Açık Mavi (0.4 Hz)
  * **+3**: Koyu Mavi (0.5 Hz)
  * **+4**: Turkuaz / Camgöbeği (0.6 Hz)
  * **+5**: Yeşil (0.7 Hz)
  * **+6**: Sarı (0.8 Hz)
  * **+7**: Turuncu (0.9 Hz)
  * **+8**: Kırmızı-Turuncu (1.0 Hz)
  * **+9**: Derin Kırmızı — hızlı, dramatik flaş (1.1 Hz)
* **Seviyeye Göre Pulse Animasyonu**: Her yükseltme kademesi kendine özgü bir hızda nefes alır. +1'de parlama neredeyse görünmez (~3.3 saniyelik döngü), +9'da ise yoğun biçimde yanıp söner (~0.9 saniyelik döngü). Parlaklık genliği de seviyeyle birlikte büyür.
* **1. Şahıs & 3. Şahıs Kamera Desteği**: Silah parlamaları hem 3. şahıs (dünya) görünümünde hem de 1. şahıs (kamera kolları) görünümünde otomatik olarak yenilenir ve mükemmel çalışır.
* **Akıllı Etkinlik Takibi**: Sıfır performans kaybı! Ağır per-frame (kare başı) döngüler yerine; kuşanma olaylarını (`TESEquipEvent`), kamera değişimlerini (`SKSE::CameraEvent`) ve envanter/kısayol menü kapanışlarını (`RE::MenuOpenCloseEvent`) dinleyerek parlamayı tamamen asenkron ve güvenli şekilde günceller.
* **Arka Plan Animasyon Thread'i**: Özel bir ~30 fps arka plan thread'i parlama zamanlayıcısını ilerletir ve her adımda ana thread'e tek bir güncelleme görevi gönderir — recursive zamanlama yok, askı (hang) yok.
* **Yay ve Kalkan Parlaması**: Yükseltilmiş yaylar ve kalkanlar da aynı animasyonlu parlama efektini alır. Glow doğru NiAV node'larına (`Bow`, `WeaponBow`, `SHIELD`) uygulanarak hem birinci hem de üçüncü şahıs görünümünde mükemmel çalışır.
* **Takipçi / Companion Parlaması**: Yükseltilmiş silah veya kalkanı takipçinize verdiğinizde, takipçinizin elindeki eşya da parlar! Sistem her animasyon adımında yakındaki (~50 metre) tüm aktörleri tarar ve doğru seviye parlamasını uygular. **NFF (Nether's Follower Framework)** ve diğer takipçi modlarıyla tam uyumludur.
* **İsim ve ExtraHealth Kalıcılık Sistemi**: Silahlarınızın seviyeleri ve hasar artışları doğrudan eşya kopyasına (`ExtraTextDisplayData` ve `ExtraHealth`) mühürlenir, oyun kaydedildiğinde ve tekrar yüklendiğinde hiçbir veri kaybı yaşanmaz.
* **Community Shaders Uyumluluğu**: *Community Shaders* (Metals ve Dynamic Cubemaps dahil) kurulu sistemlerde çökmeye neden olan klasik shader yapısı yerine doğrudan materyal ışıması (`BSLightingShaderProperty`) modifikasyonları kullanılarak %100 kararlılık sağlanmıştır.

### Güncelleme Geçmişi

#### [YENİ] Sürüm 1.1.0
* **SKSE Menu Framework (SMF) Entegrasyonu**: Tam oyun içi ayar menüsü desteği! Artık tüm mod ayarlarını (parlama açma/kapama, altın maliyet çarpanı, maksimum seviye sınırı, her seviye için başarı oranları ve parlama renkleri) SKSE Menu Framework arayüzünü kullanarak oyun içinden canlı olarak yapılandırabilirsiniz.
* **Oyun İçi INI Kaydı**: Menü içerisindeki "Save Settings to INI" butonu sayesinde yaptığınız ayarları tek tıkla `WeaponPlusSystem.ini` dosyasına kaydedebilirsiniz.

#### Version 1.0.8
* **Geliştirilmiş Dinamik Renk Geçişleri**: +7, +8 ve +9 silahların renk paletleri yüksek kontrastlı renklerle yeniden tasarlandı, böylece geçişler açıkça görülebiliyor.
  * **+7 (Zehir/Asit)**: Parlak Limon Yeşili → Koyu Pas/Kahverengi → Parlak Sarı
  * **+8 (Elektrik/Kıvılcım)**: Parlak Cyan → Koyu Lacivert → Parlak Mor
  * **+9 (Alev/Efsanevi)**: Parlak Sarı → Derin Kırmızı → Parlak Turuncu

#### [YENİ] Sürüm 1.0.6
* **Dinamik Renk Geçişleri (RGB LERP Sistemi)**: +7 ve üzeri (efsanevi seviye) silahların renkleri artık sabit kalmaz. Nefes alma efektine ek olarak, renk paletleri pürüzsüz bir şekilde birbiri içine geçerek akıcı bir görsel şölen yaratır. +1 ile +6 arasındaki renkler de uyumlu olacak şekilde yeniden kalibre edildi.
* **İsim Kalıcılığı Sistemi**: Silahların yükseltme dereceleri (örneğin `+7`) artık eşyanın özel ismine (Custom Name) mühürlenmektedir. Bu sayede oyunu kaydedip baştan yüklediğinizde yükseltme derecesi silinmez.
* **Kod ve Performans Optimizasyonu**: Kullanılmayan sistemler (Particle ArtObject denemeleri, kılıcın dokusunu bozan UV Scrolling mekanikleri vb.) tamamen temizlenmiş, SKSE animasyon döngüsü (30 FPS) optimize edilerek mod son derece hafif ve FPS dostu bir hale getirilmiştir.

#### Sürüm 1.0.3
* **INI Yapılandırma Dosyası Desteği**: Eklenti ilk kez çalıştığında `Data/SKSE/Plugins/` altında otomatik olarak `WeaponPlusSystem.ini` ayar dosyasını oluşturur. Buradan şu ayarları düzenleyebilirsiniz:
  * Yükseltme açma tuşu (`UpgradeKey`) DirectInput Hex değerleri ile (Örn: L için `0x26`).
  * Parlama efekti kontrolü (`EnableGlow`).
  * Fiyat çarpanı (`GoldCostMultiplier`).
  * Maksimum geliştirme sınırı (`MaxUpgradeLevel`).
* **Bireysel Silah Yükseltme (Tempering/ExtraHealth)**: Silah hasarını artık base (ortak) form üzerinden değil, Skyrim'in kendi bileme (tempering) yapısını taklit ederek benzersiz envanter kopyası (`ExtraHealth`) üzerinden artırır. Böylelikle yükseltilen kılıç sadece size özel olur, dünyadaki diğer kopyaların hasarı artmaz.
* **Çoklu Sürüm Uyumlu Tek DLL**: CommonLibSSE-NG altyapısıyla derleme özellikleri güncellenerek; Skyrim Special Edition **(v1.5.97)**, Anniversary Edition **(v1.6.1170)** ve **Skyrim VR** üzerinde tek bir DLL dosyasının çalışabilmesi sağlanmıştır.

### Nasıl Kullanılır?
1. Yükseltmek istediğiniz silahı veya kalkanı elinize kuşanın.
2. Herhangi bir demirci NPC'ye yaklaşın ve hedef göstergenizi ona doğrultun.
3. **L** tuşuna (veya INI dosyasında belirlediğiniz özel tuşa) basarak yükseltme ekranını açın.
4. Yeterli altınınız varsa başarı şansını kontrol edip yükseltmeyi onaylayın!
5. **Silahınızı yükselttikten sonra oyunu kaydetmeyi (F5 veya normal kayıt) unutmayın; böylece yükseltmeleriniz kalıcı olarak kaydedilecektir!**

---

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır. See [LICENSE](LICENSE.md) for details.