# Weapon Plus System (SKSE Plugin)

[TR] Türkçe açıklama aşağıdadır.

A premium, performance-optimized **Skyrim Special Edition (v1.6.1170)** SKSE C++ plugin that introduces a comprehensive weapon upgrade system. Players can level up their weapons by interacting with blacksmith NPCs, gaining both dynamic damage scaling and beautiful, tier-based animated glow effects.

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
* **Bow & Shield Glow Support**: Upgraded bows and shields also receive the full animated glow treatment — the glow is applied to the correct NiAV nodes (`Bow`, `SHIELD`) in both first- and third-person views.
* **SKSE Cosave Serialization**: Upgrade levels are fully persistent and cleanly saved and loaded inside your standard Skyrim save files.
* **Community Shaders Compatibility**: Built with custom direct material emittance overrides that bypass the standard BSEffectShader data structure, ensuring 100% crash-free stability when running *Community Shaders* (including Metals and Dynamic Cubemaps).

### How to Use
1. Equip the weapon or shield you want to upgrade.
2. Approach any blacksmith NPC and point your crosshair at them.
3. Press **L** to open the upgrade menu.
4. If you have enough gold, check the success chance and confirm the upgrade!
5. **Remember to save your game (F5 or manual save) after upgrading so your weapon upgrades persist!**

---

## Türkçe Versiyon

Skyrim Special Edition için geliştirilmiş, performans odaklı ve son teknoloji bir **SKSE C++ eklentisidir**. Oyuncuların demirci NPC'ler ile etkileşime girerek silahlarını seviyelendirmelerini, hasarlarını artırmalarını ve yükseltme seviyesine göre gerçek zamanlı animasyonlu görsel parlama efektleri kazanmalarını sağlar.

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
* **Yay ve Kalkan Parlaması**: Yükseltilmiş yaylar ve kalkanlar da aynı animasyonlu parlama efektini alır. Glow doğru NiAV node'larına (`Bow`, `SHIELD`) uygulanarak hem birinci hem de üçüncü şahıs görünümünde mükemmel çalışır.
* **SKSE Cosave Kayıt Sistemi**: Silahlarınızın seviyeleri tamamen kalıcıdır; oyunu kaydettiğinizde otomatik olarak `.ess / .cosave` kayıt dosyalarınıza yazılır ve oyunu tekrar açtığınızda sorunsuz bir şekilde yüklenir.
* **Community Shaders Uyumluluğu**: *Community Shaders* (Metals ve Dynamic Cubemaps dahil) kurulu sistemlerde çökmeye neden olan klasik shader yapısı yerine doğrudan materyal ışıması (`BSLightingShaderProperty`) modifikasyonları kullanılarak %100 kararlılık sağlanmıştır.

### Nasıl Kullanılır?
1. Yükseltmek istediğiniz silahı veya kalkanı elinize kuşanın.
2. Herhangi bir demirci NPC'ye yaklaşın ve hedef göstergenizi ona doğrultun.
3. **L** tuşuna basarak yükseltme ekranını açın.
4. Yeterli altınınız varsa başarı şansını kontrol edip yükseltmeyi onaylayın!
5. **Silahınızı yükselttikten sonra oyunu kaydetmeyi (F5 veya normal kayıt) unutmayın; böylece yükseltmeleriniz kalıcı olarak kaydedilecektir!**

---

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır. See [LICENSE](LICENSE.md) for details.